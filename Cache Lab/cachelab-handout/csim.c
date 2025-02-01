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
