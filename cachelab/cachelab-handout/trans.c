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
    // if (M == 32 && N == 32)
    // {
    //     trans32_v1(A, B);
    // }
    // else if (M == 64 && N == 64)
    // {
    //     trans64_v1(A, B);
    // }
    // else if (M == 61 && N == 67)
    // {
        trans6761_v1(A, B);
    // }
}

/**
 * @brief block size 8
 * 
 * @param A 
 * @param B 
 */
char trans32_v1_desc[] = "trans32_v1_desc";
void trans32_v1(int A[32][32], int B[32][32])
{
#define LEN 32
#define BSIZE 8
    // NOTE: 这里采用4循环是为了更好理解“blocking”机制，其实采用3个循环就能做，具体是融合j和k
    int i, j, k, q;

    // 所有循环视角从B矩阵出发
    for (i = 0; i < LEN; i += BSIZE) // 1. 以block行为单位，扫描整个矩阵
    {
        for (j = 0; j < LEN; j += BSIZE) // 2. 以一个block为单位，扫描一个block行
        {
            for (k = j; k < j + BSIZE; k++) // 3. 以一个条带为单位，扫描一个块
            {
                for (q = 0; q < BSIZE; q++) // 4. 以最小粒度为单位， 扫描一个条带
                {
                    B[i + q][k] = A[k][i + q];
                }
            }
        }
    }
#undef LEN
#undef BSIZE
}

char trans64_v1_desc[] = "trans64_v1_desc";
void trans64_v1(int A[64][64], int B[64][64])
{
#define LEN 64
#define BSIZE 4
    // NOTE: 这里采用4循环是为了更好理解“blocking”机制，其实采用3个循环就能做，具体是融合j和k
    int i, j, k, q;

    // 所有循环视角从B矩阵出发
    for (i = 0; i < LEN; i += BSIZE) // 1. 以block行为单位，扫描整个矩阵
    {
        for (j = 0; j < LEN; j += BSIZE) // 2. 以一个block为单位，扫描一个block行
        {
            for (k = j; k < j + BSIZE; k++) // 3. 以一个条带为单位，扫描一个块
            {
                for (q = 0; q < BSIZE; q++) // 4. 以最小粒度为单位， 扫描一个条带
                {
                    B[i + q][k] = A[k][i + q];
                }
            }
        }
    }
#undef LEN
#undef BSIZE
}

char trans6761_v1_desc[] = "trans6167_v1_desc";
void trans6761_v1(int A[67][61], int B[61][67])
{
    // 从A矩阵视角
#define M 67
#define N 61
#define BSIZE 4
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

    // DEBUG
    // // write b
    // FILE *fileA = fopen("AMatrix", "w");
    // FILE *fileB = fopen("BMatrix", "w");

    // char buf[100];
    // for (size_t i = 0; i < M;i++)
    // {
    //     for (size_t j = 0; j < N;j++)
    //     {
    //         sprintf(buf, "%d ", A[i][j]);
    //         fwrite(buf, 1, strlen(buf), fileA);
    //     }
    //     fwrite("\n", 1, 1, fileA);
    // }
    // fclose(fileA);

    // for (size_t i = 0; i < N;i++)
    // {
    //     for (size_t j = 0; j < M;j++)
    //     {
    //         sprintf(buf, "%d ", B[i][j]);
    //         fwrite(buf, 1, strlen(buf), fileB);
    //     }
    //     fwrite("\n", 1, 1, fileB);
    // }
    // fclose(fileB);
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
