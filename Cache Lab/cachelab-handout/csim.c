#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"

int set_index_bits = 0, lines_per_set = 0, block_offset_bits = 0;
char *tracefile = NULL;

void read_args(int argc, char *argv[])
{
    int opt;
    int verbose = 0;

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

int main(int argc, char *argv[])
{
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
    while (fgets(buffer, sizeof(buffer), file) != NULL) 
    {
        printf("%s", buffer); // 打印每一行内容
    }

    // 关闭文件
    fclose(file);
    
    // 在这里可以继续根据解析的参数执行后续的操作
    printSummary(0, 0, 0);

    return 0;
}
