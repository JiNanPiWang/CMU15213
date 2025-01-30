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
