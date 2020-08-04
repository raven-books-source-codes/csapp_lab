/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#define __USE_POSIX
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

#define LOG(...) printf(__VA_ARGS__)

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv)
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1)
    {

        /* Read command line */
        if (emit_prompt)
        {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
            app_error("fgets error");
        if (feof(stdin))
        { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline)
{
    int bg;                               // 是否是后台程序
    char buf[MAXLINE];                    // cmd copy
    char *argv[MAXARGS];                  // 解析后的命令
    pid_t pid = -1;                       // 子进程id
    sigset_t mask_all, mask_one, pre_one; // 用于同步

    strcpy(buf, cmdline);
    bg = parseline(cmdline, argv);
    if (argv[0] == NULL)
        return;

    // 初始化blocking mask
    sigfillset(&mask_all);
    sigemptyset(&mask_one);
    sigaddset(&mask_one, SIGCHLD);

    // 正式执行命令
    if (!builtin_cmd(argv))
    {
        // 外部命令
        if (access(argv[0], F_OK) == -1)
        {
            char msg[MAXLINE];
            sprintf(msg, "%s: Command not found\n", argv[0]);
            printf(msg);
            return;
        }

        // 能够运行的外部命令
        // Parent: 在fork前，block SIGCHLD 信号，避免在addjob之前Child结束，触发sigchld_handler,从而deltejob比addjob先执行
        sigprocmask(SIG_BLOCK, &mask_one, &pre_one);
        if ((pid = fork()) == 0)
        {                  // Child
            setpgid(0, 0); // 改变组id，避免信号的自动传送(write up中有更相似的描述)
            // 执行exec之前，解除从Parent中继承过来的block_mask
            sigprocmask(SIG_SETMASK, &pre_one, NULL);
            // 执行
            if (execve(argv[0], argv, environ) == -1)
            {
                unix_error("execve child process error");
            }
        }
        else
        { // Parent
            // addjob, block所有信号，避免竞争问题
            sigprocmask(SIG_SETMASK, &mask_all, NULL);
            addjob(jobs, pid,
                   bg ? BG : FG,
                   buf);
            // 还原信号
            sigprocmask(SIG_SETMASK, &pre_one, NULL);
        }

        // Parent: 前台进程处理
        if (!bg)
        {
            waitfg(pid);
        }
        else
        {
            // TODO: 在这里输出，耦合了addjob函数(内部对nextjid++)，如何优化？
            printf("[%d] (%d) %s &\n", nextjid - 1, pid, argv[0]);
        }
    }
    return;
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv)
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
        buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'')
    {
        buf++;
        delim = strchr(buf, '\'');
    }
    else
    {
        delim = strchr(buf, ' ');
    }

    while (delim)
    {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* ignore spaces */
            buf++;

        if (*buf == '\'')
        {
            buf++;
            delim = strchr(buf, '\'');
        }
        else
        {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0)
    {
        argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv)
{
    char *cmd = argv[0];
    // 检查合法性
    if (!cmd || !strlen(cmd))
    {
        printf("Cmmond not found");
    }

    // 判定4种内置命令
    if (!strcmp("quit", cmd))
    {
        // 考虑当前还有后台进程，需要kill
        sigset_t mask_all, pre_one;
        sigfillset(&mask_all);
        sigprocmask(SIG_SETMASK, &mask_all, &pre_one);
        // 先kill 所有子进程
        for (int i = 0; i < MAXJOBS; i++)
        {
            if (jobs[i].pid)
            {
                kill(jobs[i].pid, SIGKILL);
            }
        }
        clearjob(jobs);
        sigprocmask(SIG_SETMASK, &pre_one, NULL);
        // 退出
        exit(0);
    }
    else if (!strcmp("jobs", cmd))
    {
        listjobs(jobs);
    }
    else if (!strcmp("fg", cmd) || !strcmp("bg", cmd))
    {
        do_bgfg(argv);
    }
    else
    {
        return 0; /* not a builtin command */
    }
    return 1;
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
    // NOTE:本函数假设argv[0] 只能 = fg 或者 = bg
    char *arg = argv[1]; // 命令参数
    int jid = -1, pid = -1;
    struct job_t *job;
    sigset_t mask_all, pre_one;

    // 命令是否有效？
    if (argv[1] == NULL)
    {
        char msg[MAXLINE];
        sprintf(msg, "%s command requires PID or %%jobid argument\n", argv[0]);
        printf(msg);
        return;
    }

    if (arg[0] != '%')
    { // pid 模式
        pid = atoi(arg);
        jid = pid2jid(pid);
    }
    else
    { // jid 模式
        jid = atoi(arg + 1);
    }

    job = getjobjid(jobs, jid);
    if (!job)
    {
        if (arg[0] != '%')
        {
            printf("(%d): No such process\n", pid);
        }
        else
        {
            printf("%%%d: No such job\n", jid);
        }
        return;
    }

    sigprocmask(SIG_SETMASK, &mask_all, &pre_one);
    if (!strcmp("fg", argv[0]))
    { // fg mode
        // 后台进程转前台
        if (job->state == FG)
        {
            printf("process is foreground already\n");
            return;
        }

        // ST -> RUNNING
        kill(job->pid, SIGCONT);

        job->state = FG;
        waitfg(job->pid);
    }
    else if (!strcmp("bg", argv[0]))
    { // bg mode
        // 后台stop进程转running进程
        if (job->state != ST)
        {
            printf("process isn't stoped\n");
            return;
        }

        kill(job->pid, SIGCONT);
        job->state = BG;
        // TODO: 这里的printf后面没有加\n，因为cmdline自带\n
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
    sigprocmask(SIG_SETMASK, &pre_one, 0);

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // Note: 因为前台进程结束后，会自动调用chld_handler,所以回收工作交给chld_handler处理
    // 这里只用保证tsh被阻塞即可。
    sigset_t mask_all, pre_one;
    // 初始化blocking mask
    sigfillset(&mask_all);

    while (1)
    { // 从全局变量jobs中读取 fg pid，避免jobs的竞争
        sigprocmask(SIG_SETMASK, &mask_all, &pre_one);
        if (!fgpid(jobs))
        {
            // 没有前台进程
            break;
        }
        sigprocmask(SIG_SETMASK, &pre_one, 0);
        sleep(1);
    }
    sigprocmask(SIG_SETMASK, &pre_one, 0);
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig)
{
    int olderrno = errno;
    sigset_t mask_all, pre_one;
    int pid, wstate;
    struct job_t *job; // 触发本次sigchld_handler的job

    sigfillset(&mask_all);

    pid = waitpid(-1, &wstate, WNOHANG | WUNTRACED);

    job = getjobpid(jobs, pid);
    if (!job)
    {
        printf("job not found\n");
        return;
    }

    // waitpid调用返回有两种情况：
    // 1. 子进程terminate
    // 2. 通过信号，被stop了，如用户键入 ctrl+z
    sigprocmask(SIG_SETMASK, &mask_all, &pre_one);
    if (WIFSTOPPED(wstate))
    { // stoped，更新状态即可
        job->state = ST;
    }
    else
    {   // terminate，需要deletejob
        deletejob(jobs, pid);
    }
    sigprocmask(SIG_SETMASK, &pre_one, NULL);
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig)
{
    int olderrno = errno;
    int pid;
    struct job_t *job;

    pid = fgpid(jobs);
    if(!pid){
        return;
    }
    job = getjobpid(jobs,pid);
    if(!job){
        return;
    }

    // TODO: 不应该在handler中出现printf这类async-unsafe, 替换为safe-library即可
    printf("Job [%d] (%d) terminated by signal %d\n", job->jid, job->pid, sig);

    // 有前台进程, forward to it， 后续处理交给chld_handler
    // 注意传递INT信号可能会造成死循环：具体参考https://blog.csdn.net/guozhiyingguo/article/details/53837424
    kill(pid, SIGINT);

    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{
    int olderrno = errno;
    int pid;
    struct job_t *job;

    pid = fgpid(jobs);
    if(!pid){
        return;
    }

    job = getjobpid(jobs,pid);
    if(!job){
        return;
    }


    // TODO: 不应该在handler中出现printf这类async-unsafe, 替换为safe-library即可
    printf("Job [%d] (%d) stoped by signal %d\n", job->jid, job->pid, sig);

    // 有前台进程, forward to it， 后续处理交给chld_handler
    // 注意传递INT信号可能会造成死循环：具体参考https://blog.csdn.net/guozhiyingguo/article/details/53837424
    kill(pid, SIGTSTP);

    errno = olderrno;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
        {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
