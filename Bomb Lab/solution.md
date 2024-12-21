## bomb1：

objdump -d ./bomb 查看反汇编代码

然后看phase_1函数，

```asm
0000000000400ee0 <phase_1>:
  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>
  400eee:	85 c0                	test   %eax,%eax
  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
  400ef7:	48 83 c4 08          	add    $0x8,%rsp
  400efb:	c3                   	retq   
```

可以看到，phase_1函数调用了strings_not_equal函数，然后根据返回值判断是否爆炸。

strings_not_equal函数的一个参数是0x402400，另一个参数是%rdi寄存器，也就是我们输入的字符串。

根据常理，字符串比较的参数都是指针，所以我们比较的内容应该是0x402400指向的内容。

再使用gdb调试，查看0x402400的内容就结束了

## bomb2：

逐步分析反汇编代码，我们输入的内容同样是存储在%rdi寄存器中，目前，%rsp也就是栈帧的值是0x7fffffffdce8

```asm
0000000000400efc <phase_2>:
  400efc:	55                   	push   %rbp
  400efd:	53                   	push   %rbx
  400efe:	48 83 ec 28          	sub    $0x28,%rsp
  400f02:	48 89 e6             	mov    %rsp,%rsi
```

push操作会自动把%rsp栈帧寄存器移动，上面的操作相当于是给栈分配了40个字节的空间，再把当前的栈帧存入%rsi寄存器，%rsp减0x28变为0x7fffffffdcc0

```asm
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
```

调用了read_six_numbers函数，因为调用函数需要保存当前的地址，用于等一下返回用，所以%rsp需要减8，分配8个字节的空间用于存储返回到哪里，现在%rsp减0x8变为了0x7fffffffdcb8。现在是分配了48个字节的空间。

```asm
000000000040145c <read_six_numbers>:
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
```

调用再分配了24个字节的空间，目前总共是在栈上分配了72个字节的空间，%rsp减0x18变为了0x7fffffffdca0，然后将栈40个字节的指针存入rdx寄存器

```asm
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
```

lea负责取地址，然而0x4(%rsi)代表取(%rsi+4)地址的值，所以相当于把%rsi+4存入了%rcx，因为%rsi是，也就是栈帧36字节的地方。同理，%rax存的就是栈帧20字节地址，也就是%rsi+20

```asm
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
```

栈帧56字节存入栈帧20字节地址，也就是栈的56~64字节存的是