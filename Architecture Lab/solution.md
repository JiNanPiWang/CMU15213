运行前，请先执行

```bash
cd sim
make clean
make
```

## arch1

partA的第一题，要我们用Y86汇编写出下面这个c语言程序。

```c
/* linked list element */
typedef struct ELE {
    long val;
    struct ELE *next;
} *list_ptr;

/* sum_list - Sum the elements of a linked list */
long sum_list(list_ptr ls)
{
    long val = 0;
    while (ls) {
		val += ls->val;
		ls = ls->next;
    }
    return val;
}
```

结构体内部是一个连续的地址

```asm
# sum.ys: 迭代地计算链表的元素和
# Name: [你的名字]
# ID: [你的学号]

.pos 0
# 程序入口
    irmovq Stack, %rsp      # 初始化栈指针，Stack定义在下面
	irmovq ele1, %rdi       # 将链表头指针传入 %rdi
	call sum_list           # 调用 sum_list 函数
	halt                    # 程序结束

# 链表元素定义
.align 8
ele1:
	.quad 0x00a				# ele1 的值
	.quad ele2				# ele1 的下一个节点地址
ele2:
	.quad 0x0b0				# ele2 的值
	.quad ele3				# ele2 的下一个节点地址
ele3:
	.quad 0xc00				# ele3 的值
	.quad 0					# ele3 的下一个节点地址（终止）


sum_list:
	pushq %rsi
    irmovq 0,%rax			# 返回值=0
loop:
    andq %rdi,%rdi			# 判断参数是否为0了
    je end
    mrmovq (%rdi),%rsi		# val += ls->val
	addq %rsi,%rax
    mrmovq 8(%rdi),%rdi		# ls = ls->next
    jmp loop
end:
	popq %rsi
    ret

# 栈空间定义
.pos 0x100                  # 栈空间从 .pos 0x100 开始，以保证程序数据和栈数据不会冲突。
.align 8
Stack:                      # 栈底地址

```

### **运行结果验证**

1. 汇编：
   ```bash
   yas sum.ys
   ```

2. 模拟运行：
   ```bash
   yis sum.yo
   ```

3. 查看 `%rax` 值是否为：
   ```
   0x00a + 0x0b0 + 0xc00 = 0x0cba
   ```


## arch2