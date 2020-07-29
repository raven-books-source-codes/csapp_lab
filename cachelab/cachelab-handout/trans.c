/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

/***********************自定义函数声明************************/
void trans32_v1(int A[32][32], int B[32][32]);
void trans64_v1(int A[64][64], int B[64][64]);
void trans6761_v1(int A[67][61], int B[61][67]);

char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32)
    {
        trans32_v1(A, B);
    }
    else if (M == 64 && N == 64)
    {
        trans64_v1(A, B);
    }
    else if (M == 61 && N == 67)
    {
        trans6761_v1(A, B);
    }
}

/**
 * @brief block size 8
 * miss计算:
 * BSIZE 8， 总共分为了16块， 假设块编号从0开始，由左向右，由上向下。
 * 则左边第一列块中的每一块（块编号为0,4,8,12)miss数为(9+7+7)：
 * 1.第1个9代表 load A的第一个条带(1个miss)+ store B的第一个条带（8个miss）
 * 2.第2个7代表 load A的剩余7列带来的miss
 * 3.第3个7代表，store B的剩余7列带来的miss
 * 具体可参见文件： m32n32-23miss
 * 所以这4个块总miss为：4*(9+7+7) = 92
 * 
 * 上述说完了最靠左边的一列block，还剩3列block，3*4 = 12个block
 * 这些block的共性就是 load A条带不会和Store A条带冲突。
 * 所以扫描完一个block会造成的miss为8+8：
 * 1. 第一个8代表，store B的第一个条带（8个miss）
 * 2. 第二个8代表， load 8个A条带带来的miss
 * 所以这12个块总miss为： 12*(8+8) = 192
 * 具体可参见文件：m32n32-16miss
 * 
 * 另外还有3个固定的额外开销，具体对应哪个cache miss未知，猜测为初始化循环变量带来的（但应该不是，如果你知道，还请告诉我）
 * 具体可参见文件：m32n32-extra-miss
 * 
 * 所以最终的miss为:
 * 92+192+3 = 287
 * @param A 
 * @param B 
 */
char trans32_v1_desc[] = "trans32_v1_desc";
void trans32_v1(int A[32][32], int B[32][32])
{
#define LEN 32
#define BSIZE 8

    int i, j, k;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    // 所有循环视角从B矩阵出发
    for (i = 0; i < LEN; i += BSIZE) // 1. 以block行为单位，扫描整个矩阵
    {
        for (j = 0; j < LEN; j += BSIZE) // 2. 以一个block为单位，扫描一个block行
        {
            for (k = j; k < j + BSIZE; k++) // 3. 以一个条带为单位，扫描一个块
            {

                // 避免对角线的 A B矩阵cacheline竞争
                t0 = A[k][i + 0];
                t1 = A[k][i + 1];
                t2 = A[k][i + 2];
                t3 = A[k][i + 3];
                t4 = A[k][i + 4];
                t5 = A[k][i + 5];
                t6 = A[k][i + 6];
                t7 = A[k][i + 7];

                B[i + 0][k] = t0;
                B[i + 1][k] = t1;
                B[i + 2][k] = t2;
                B[i + 3][k] = t3;
                B[i + 4][k] = t4;
                B[i + 5][k] = t5;
                B[i + 6][k] = t6;
                B[i + 7][k] = t7;

                // 第二种写法,但是存在对角线竞争
                // for (q = 0; q < BSIZE; q++) // 4. 以最小粒度为单位， 扫描一个条带
                // {
                //     B[i + q][k] = A[k][i + q];
                // }
            }
        }
    }
#undef LEN
#undef BSIZE
}

/**
 * @brief block size 8
 * 
 * @param A 
 * @param B 
 */
char trans64_v1_desc[] = "trans64_v1_desc";
void trans64_v1(int A[64][64], int B[64][64])
{
#define LEN 64
    int i, j, k;
    int t0, t1, t2, t3, t4, t5, t6, t7;

    // 所有循环视角从B矩阵出发
    for (i = 0; i < LEN; i += 8) // 1. 以block行为单位，扫描整个矩阵
    {
        for (j = 0; j < LEN; j += 8) // 2. 以一个block为单位，扫描一个block行
        {
            for (k = j; k < j + 8; k++) // 3. 以一个条带为单位，扫描一个块
            {

                t0 = A[k][i + 0];
                t1 = A[k][i + 1];
                t2 = A[k][i + 2];
                t3 = A[k][i + 3];
                t4 = A[k][i + 4];
                t5 = A[k][i + 5];
                t6 = A[k][i + 6];
                t7 = A[k][i + 7];

                if (j == 0)
                {
                    // 第一块
                    // 前4个正常放置
                    B[i + 0][k] = t0;
                    B[i + 1][k] = t1;
                    B[i + 2][k] = t2;
                    B[i + 3][k] = t3;

                    // 后4个需要单独处理
                    B[i + k % 4][k - k % 4 + 8] = t4;
                    B[i + k % 4][k - k % 4 + 9] = t5;
                    B[i + k % 4][k - k % 4 + 10] = t6;
                    B[i + k % 4][k - k % 4 + 11] = t7;
                }
                else
                {
                    // 非第一块，首先需要将元素搬迁到正确的位置
                    if (k == j) // 第一次进入该块时，搬迁
                    {
                        for (size_t a = i; a < i + 4; a++)
                        {
                            for (size_t b = j; b < j + 4; b++) // 一个条带
                            {
                                // 两点关于一条直线对称
                                B[b + i - j + 4][a - i + j - 8] = B[a][b];
                                B[b + i - j + 4][a - i + j - 4] = B[a][b + 4];
                            }
                        }
                    }

                    // 然后，填入本block正确的元素
                    if (j != LEN - 8)
                    { 
                        // 中间块处理方式和第一块处理方式相同
                        // 前4个正常放置
                        B[i + 0][k] = t0;
                        B[i + 1][k] = t1;
                        B[i + 2][k] = t2;
                        B[i + 3][k] = t3;

                        // 后4个需要单独处理
                        B[i + k % 4][k - k % 4 + 8] = t4;
                        B[i + k % 4][k - k % 4 + 9] = t5;
                        B[i + k % 4][k - k % 4 + 10] = t6;
                        B[i + k % 4][k - k % 4 + 11] = t7;
                    }
                    else
                    {
                        // 最后一块单独处理
                        // 只处理8x8的上 4x8方块
                        B[i + 0][k] = t0;
                        B[i + 1][k] = t1;
                        B[i + 2][k] = t2;
                        B[i + 3][k] = t3;
                    }
                }
            }
        }
        // 补齐最后一块的下4行条带， 即最后的8x8的下4x8方块
        for (size_t a = 56; a < 64; a++)
        {
            t4 = A[a][i + 4];
            t5 = A[a][i + 5];
            t6 = A[a][i + 6];
            t7 = A[a][i + 7];

            B[i + 4][a] = t4;
            B[i + 5][a] = t5;
            B[i + 6][a] = t6;
            B[i + 7][a] = t7;
        }
    }
#undef LEN
}

char trans6761_v1_desc[] = "trans6167_v1_desc";
void trans6761_v1(int A[67][61], int B[61][67])
{
    // 从A矩阵视角
#define M 67
#define N 61
#define BSIZE 16
    int i, j, k, q;

    // 所有循环视角从B矩阵出发
    for (i = 0; i < N; i += BSIZE) // 1. 以block行为单位，扫描整个矩阵
    {
        for (j = 0; j < M; j += BSIZE) // 2. 以一个block为单位，扫描一个block行
        {
            for (k = j; k < j + BSIZE; k++) // 3. 以一个条带为单位，扫描一个块
            {
                for (q = 0; q < BSIZE; q++) // 4. 以最小粒度为单位， 扫描一个条带
                {
                    if (k < M && i + q < N)
                    {
                        B[i + q][k] = A[k][i + q];
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
#undef N
#undef M
#undef BSIZE
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{

    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; ++j)
        {
            if (A[i][j] != B[j][i])
            {
                return 0;
            }
        }
    }
    return 1;
}
