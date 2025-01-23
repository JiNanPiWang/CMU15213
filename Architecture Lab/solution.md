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

结构体内部是一个连续的地址，