因为我们在地址内存东西，是从小地址存到大地址，比如一个int32，就是从0x100到0x103，所以栈分配空间就是往小分配，rsp减才是分配空间

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

push操作会自动把%rsp栈帧寄存器移动，上面的操作相当于是给栈分配了8+8+40=56个字节的空间，再把当前的栈帧存入%rsi寄存器，%rsp减0x28变为0x7fffffffdcc0，%rsi=0x7fffffffdcc0

```asm
  400f05:	e8 52 05 00 00       	callq  40145c <read_six_numbers>
```

调用了read_six_numbers函数，因为调用函数需要保存当前的地址，用于等一下返回用，所以%rsp需要减8，分配8个字节的空间用于存储返回到哪里，现在%rsp减0x8变为了0x7fffffffdcb8。现在是分配了64个字节的空间。

```asm
000000000040145c <read_six_numbers>:
  40145c:	48 83 ec 18          	sub    $0x18,%rsp
  401460:	48 89 f2             	mov    %rsi,%rdx
```

调用再分配了24个字节的空间，目前总共是在栈上分配了88个字节的空间，%rsp减0x18变为了0x7fffffffdca0，然后将栈40个字节的指针存入rdx寄存器

```asm
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
```

lea负责取地址，然而0x4(%rsi)代表取(%rsi+4)地址的值，所以相当于把%rsi+4存入了%rcx，因为%rsi是，也就是栈帧36字节的指针。同理，%rax存的就是栈帧20字节地址，也就是%rsi+20

```asm
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
```

栈帧第-80字节存入栈帧第-20字节地址，也就是栈的-80~-73字节存的是0x7fffffffdce0。

```asm
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
```

%rax存入%rsi+0x10，值就是0x7fffffffdcd0。栈的-88到-81字节存的就是0x7fffffffdcd0。

```asm
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
```

%r9存0x7fffffffdccc，%r8存0x7fffffffdcc8，%esi存0x4025c3，%eax存0，然后调用400bf0的函数

```asm
0000000000400bf0 <__isoc99_sscanf@plt>:
  400bf0:	ff 25 92 24 20 00    	jmpq   *0x202492(%rip)        # 603088 <__isoc99_sscanf@GLIBC_2.7>
  400bf6:	68 11 00 00 00       	pushq  $0x11
  400bfb:	e9 d0 fe ff ff       	jmpq   400ad0 <.plt>
```

调用之后，%rsp再-8，96个字节，到了0x7fffffffdc98。这里的 %rip 指向的是这条指令的下一条指令（400bf6），目标地址 = 0x400bf6 + 0x202492 = 0x603088，gdb查看0x603088的值是400bf6，带 * 的 jmpq是间接跳转，也就是跳向603088的内容，也就是下一行...。

再向栈中推入0x11这个数，%rsp再-8，104个字节，到了0x7fffffffdc90，再跳向400ad0

```asm
0000000000400ad0 <.plt>:
  400ad0:	ff 35 1a 25 20 00    	pushq  0x20251a(%rip)        # 602ff0 <_GLOBAL_OFFSET_TABLE_+0x8>
  400ad6:	ff 25 1c 25 20 00    	jmpq   *0x20251c(%rip)        # 602ff8 <_GLOBAL_OFFSET_TABLE_+0x10>
  400adc:	0f 1f 40 00          	nopl   0x0(%rax)  
```

%rip的值在cpu取址阶段就立刻改变，所以第一条是向栈里推进0x20251a+400ad6=602ff0，%rsp再-8，112个字节。第二条，是跳转到602ff8对应的值的地点，动态链接，使用x/i $rip可以看出来下一个要执行的命令，使用stepi可以跳到下一条指令去

```asm
  push  %rbx
  mov   %rsp,%rbx
  and   $0xffffffffffffffc0,%rsp
```
第一条把0放入栈，栈已经120个字节了。再把当前120字节位置的栈帧存入%rbx。再将栈帧前面全变成1，保留最后的六位，我发现不同的机器上最后几位还是一样的，所以这里合理。rsp变为0x7fffffffe240

```asm
  sub   0x211f09(%rip),%rsp       #0x7ffff7ffc808 <_rtld_global_ro+168>
  mov   %rax,(%rsp)
  mov   %rcx,0x8(%rsp)
```

第一条sub命令让%rsp减去0x7ffff7ffc808指向的值，gdb查看是0x380，十进制的896，相当于现在分配了1016字节了，然后把0放入栈顶，