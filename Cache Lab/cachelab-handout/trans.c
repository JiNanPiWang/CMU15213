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
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */

// 8x8的时候，所有的cache line可以存下8行所有的内容，不用切换
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    const int BLOCK_SIZE = 8;
    int i, j;
    int a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10;
    if (M == 32)
    {
        for (i = 0; i < N; i += BLOCK_SIZE) 
        {
            for (j = 0; j < M; j += BLOCK_SIZE) 
            {
                for (int ii = i; ii < i + BLOCK_SIZE; ++ii)
                {
                    a0 = A[ii][j];
                    a1 = A[ii][j + 1];
                    a2 = A[ii][j + 2];
                    a3 = A[ii][j + 3];
                    a4 = A[ii][j + 4];
                    a5 = A[ii][j + 5];
                    a6 = A[ii][j + 6];
                    a7 = A[ii][j + 7];

                    B[j][ii] = a0;
                    B[j + 1][ii] = a1;
                    B[j + 2][ii] = a2;
                    B[j + 3][ii] = a3;
                    B[j + 4][ii] = a4;
                    B[j + 5][ii] = a5;
                    B[j + 6][ii] = a6;
                    B[j + 7][ii] = a7;
                }
            }
        }  
    }
    else if (M == 64)
    {
        const int BLOCK_SIZE = 4;
        for (i = 0; i < N; i += BLOCK_SIZE) 
        {
            for (j = 0; j < M; j += BLOCK_SIZE) 
            {
                for (int ii = i; ii < i + BLOCK_SIZE; ++ii)
                {
                    a0 = A[ii][j];
                    a1 = A[ii][j + 1];
                    a2 = A[ii][j + 2];
                    a3 = A[ii][j + 3];

                    B[j][ii] = a0;
                    B[j + 1][ii] = a1;
                    B[j + 2][ii] = a2;
                    B[j + 3][ii] = a3;
                }
            }
        }
    }
    else if (M == 61)
    {
        const int BLOCK_SIZE = 11;
        for (i = 0; i < N / BLOCK_SIZE * BLOCK_SIZE; i += BLOCK_SIZE) 
        {
            for (j = 0; j < M / BLOCK_SIZE * BLOCK_SIZE; j += BLOCK_SIZE) 
            {
                for (int ii = i; ii < i + BLOCK_SIZE; ++ii)
                {
                    a0 = A[ii][j];
                    a1 = A[ii][j + 1];
                    a2 = A[ii][j + 2];
                    a3 = A[ii][j + 3];
                    a4 = A[ii][j + 4];
                    a5 = A[ii][j + 5];
                    a6 = A[ii][j + 6];
                    a7 = A[ii][j + 7];
                    a8 = A[ii][j + 8];
                    a9 = A[ii][j + 9];
                    a10 = A[ii][j + 10];

                    B[j][ii] = a0;
                    B[j + 1][ii] = a1;
                    B[j + 2][ii] = a2;
                    B[j + 3][ii] = a3;
                    B[j + 4][ii] = a4;
                    B[j + 5][ii] = a5;
                    B[j + 6][ii] = a6;
                    B[j + 7][ii] = a7;
                    B[j + 8][ii] = a8;
                    B[j + 9][ii] = a9;
                    B[j + 10][ii] = a10;
                }
            }
        }  
        // 处理剩余的列（如果有的话，处理不完整的块）
        for (i = 0; i < N / BLOCK_SIZE * BLOCK_SIZE; i++) 
        {
            for (j = M / BLOCK_SIZE * BLOCK_SIZE; j < M; j++) 
            {
                a0 = A[i][j];
                B[j][i] = a0;
            }
        }
        // 处理剩余的行（如果有的话，处理不完整的块）
        for (i = N / BLOCK_SIZE * BLOCK_SIZE; i < N; i++) 
        {
            for (j = 0; j < M / BLOCK_SIZE * BLOCK_SIZE; j++) 
            {
                a0 = A[i][j];
                B[j][i] = a0;
            }
        }
        // 处理尾部（非完全块部分）
        for (i = N / BLOCK_SIZE * BLOCK_SIZE; i < N; i++) 
        {
            for (j = M / BLOCK_SIZE * BLOCK_SIZE; j < M; j++) 
            {
                a0 = A[i][j];
                B[j][i] = a0;
            }
        }
    }
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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
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

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

