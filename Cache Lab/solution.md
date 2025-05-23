## cache1
要实现一个cache计数器

你给的例子是一个通过 `csim-ref` 进行缓存模拟的输出，包括普通模式和详细模式（verbose mode）下的输出。我们可以从这些输出中进一步分析整个过程和每个操作的结果。

### 分析命令
```bash
linux> ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace
```

这个命令的含义是：
- `-s 4`：使用 `s=4`，即缓存有 \( 2^4 = 16 \) 个组（sets）。
- `-E 1`：每个集合只有 1 行（1-way associative）。
- `-b 4`：每个块大小为 \( 2^4 = 16 \) 字节。
- `-t traces/yi.trace`：使用 `traces/yi.trace` 文件作为输入追踪文件。

### 分析缓存配置
- 16 个集合，1 行每个集合，块大小 16 字节。这个配置是直接映射缓存。
- 每个内存地址将直接映射到一个唯一的集合中。

### 详细输出解析
假设追踪文件 `traces/yi.trace` 包含以下内存访问操作：

```
L 10,1
M 20,1
L 22,1
S 18,1
L 110,1
L 210,1
M 12,1
```

这些地址写成二进制是：
```
0000 0001 0000, load     miss
0000 0010 0000, modify   miss hit
0000 0010 0010, load     hit
0000 0001 1000, store    hit
0001 0001 0000, load     miss eviction
0010 0001 0000, load     miss eviction
0000 0001 0010, modify   miss eviction hit
```

```
 L 10,1 miss
 M 20,1 miss hit
 L 22,1 hit
 S 18,1 hit
 L 110,1 miss eviction
 L 210,1 miss eviction
 M 12,1 miss eviction hit
```

我们需要倒着看，最后对应的是某组某行内的块编号，4位，再向左数4位，就是组编号，再前面的内容就是tag。

### 最终统计结果
- **命中数**：4
- **未命中数**：5
- **驱逐数**：3

驱逐方案就是LRU，也就是把最久没用的驱逐走。

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"

int set_index_bits, lines_per_set, block_offset_bits, verbose;
int miss_count, hit_count, evict_count;
char *tracefile = NULL;

struct Cache
{
    int time;
    unsigned long tag;
};
struct Cache cache[32][5]; // set times line

void read_args(int argc, char *argv[])
{
    int opt;

    // 解析命令行参数
    // getopt 函数会从 argv 中提取命令行参数。你通过指定参数字符串来告诉它需要处理的选项。
    // "hv:s:E:b:t:" 表示：
    // -h 和 -v 是标志选项，不需要附加值；
    // -s, -E, -b, 和 -t 后面需要一个值，optarg 会指向该值。
    while ((opt = getopt(argc, argv, "hv:s:E:b:t:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            // 打印帮助信息
            printf("Usage: ./program [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
            printf(" -h: Help flag\n");
            printf(" -v: Verbose mode\n");
            printf(" -s <s>: Set index bits (S = 2^s)\n");
            printf(" -E <E>: Number of lines per set (associativity)\n");
            printf(" -b <b>: Block bits (B = 2^b)\n");
            printf(" -t <tracefile>: Name of the Valgrind trace file\n");
            exit(0);
        case 'v':
            // 启用详细模式
            verbose = 1;
            break;
        case 's':
            // 解析集合索引位数
            set_index_bits = atoi(optarg);
            break;
        case 'E':
            // 解析每个集合的行数（组关联度）
            lines_per_set = atoi(optarg);
            break;
        case 'b':
            // 解析块位数
            block_offset_bits = atoi(optarg);
            break;
        case 't':
            // 解析追踪文件
            tracefile = optarg;
            break;
        default:
            // 处理未知选项
            fprintf(stderr, "Usage: %s [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
            exit(1);
        }
    }

    // 确保所有必须的参数都有提供
    if (set_index_bits == -1 ||
        lines_per_set == -1 ||
        block_offset_bits == -1 ||
        tracefile == NULL)
    {
        fprintf(stderr, "Error: Missing required parameters\n");
        exit(1);
    }
}

// 创建一个掩码，用于提取64位数第 x 位到第 y 位
unsigned long bits_mask(int x, int y)
{
    unsigned long mask = -1;
    return ((mask >> x) & (mask << (63 - y)));
}

void get_id(unsigned long addr, unsigned long *set_id, unsigned long *block_id, unsigned long *tag)
{
    *block_id = addr & bits_mask(63 - block_offset_bits + 1, 63);

    *set_id = (addr &
               bits_mask(63 - block_offset_bits - set_index_bits + 1, 63 - block_offset_bits)) >>
              (block_offset_bits);
    *tag = (addr &
            bits_mask(0, 63 - block_offset_bits - set_index_bits)) >>
           (block_offset_bits + set_index_bits);
}

int evict(int set_id)
{
    int min_time = cache[set_id][0].time, min_num = 0;
    for (int i = 0; i < lines_per_set; ++i)
    {
        if (cache[set_id][i].time == -1)
        {
            return i;
        }
        if (cache[set_id][i].time < min_time)
        {
            min_time = cache[set_id][i].time;
            min_num = i;
        }
    }
    ++evict_count;
    return min_num;
}

void trace_process(char *trace, int num_process)
{
    if (trace[1] != 'L' && trace[1] != 'M' && trace[1] != 'S')
        return;

    char type;
    unsigned long addr, set_id, block_id, tag;
    int len;
    sscanf(trace, " %c %lx,%d", &type, &addr, &len);
    get_id(addr, &set_id, &block_id, &tag);

    // 特判Modify，拆分成load和store
    if (type == 'M')
    {
        trace[1] = 'L';
        trace_process(trace, num_process);
        trace[1] = 'S';
        trace_process(trace, num_process);
        return;
    }

    // 判断addr对应的内容在不在cache里面，遍历line，查看tag是否在其中
    int found_trace_in_cache = 0;
    for (int i = 0; i < lines_per_set; ++i)
    {
        if (cache[set_id][i].tag == tag && cache[set_id][i].time != -1)
        {
            ++hit_count;
            cache[set_id][i].time = num_process;
            found_trace_in_cache = 1;
            break;
        }
    }

    if (found_trace_in_cache)
        return;
    ++miss_count;
    int free_line = evict(set_id);
    cache[set_id][free_line].time = num_process;
    cache[set_id][free_line].tag = tag;

    // printf("Type: %c, addr: %lx, len: %d\n", type, addr, len);
    printf("Set ID: %lu, Block ID: %lu, Tag: %lu\n", set_id, block_id, tag);
}

void init()
{
    for (int i = 0; i < 32; ++i)
        for (int j = 0; j < 5; ++j)
        {
            cache[i][j].time = -1;
            cache[i][j].tag = -1;
        }
}

int main(int argc, char *argv[])
{
    init();
    read_args(argc, argv);

    // 打印解析的参数值
    printf("Verbose mode: %s\n", verbose ? "Enabled" : "Disabled");
    printf("Set index bits: %d, Lines per set: %d, Block offset bits: %d, Tracefile: %s\n",
           set_index_bits, lines_per_set, block_offset_bits, tracefile);

    FILE *file;
    char buffer[256]; // 用于存储每行内容的缓冲区

    // 打开文件
    file = fopen(tracefile, "r");
    if (file == NULL)
    {
        perror("Error opening file"); // 如果文件打开失败，打印错误信息
        return EXIT_FAILURE;
    }
    // 逐行读取文件内容
    int num_process = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        trace_process(buffer, num_process);
        ++num_process;
        printf("Miss count: %d, Hit count: %d, Evict count: %d\n", miss_count, hit_count, evict_count);
    }

    // 关闭文件
    fclose(file);

    // 在这里可以继续根据解析的参数执行后续的操作
    printSummary(hit_count, miss_count, evict_count);

    return 0;
}
```

## cache2

我们的cache是有32个组，每组有1行，每行存32个字节的内容。也就是10位，地址后10位之前的内容都是tag。

32x32的时候，每一行需要32x4个字节，也就是4个组能存一行。第0行开始地址是0，第一行就是32x4字节，第四个cache组，第8行就是32x32字节，第8行就能到cache组的最后，然后第九行就回到cache组的最上面，这样会导致cache冲突，替换，访问B的时候就会这样。所以8行之内，我们可以保证cache组稳定，不变化。所以在32x32的时候，我们分块8x8，一次只处理8行，那么访问B的每一列，我们都可以保证cache组稳定，不变化。

同理，64x64的时候，每一行需要64x4个字节，一样的计算，我们分块大小需要变成4x4。拿满分需要8x8块内再分块4x4

而61x67的，因为都是质数，所以cache组会错开，分块尽量大一点就行了。但是哪个冲突次数最少得试，分块11的时候就满分了。

```c
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
```

```
Cache Lab summary:
                        Points   Max pts      Misses
Csim correctness          27.0        27
Trans perf 32x32           8.0         8         287
Trans perf 64x64           3.4         8        1699
Trans perf 61x67          10.0        10        1931
          Total points    48.4        53
```