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

调用再分配了24个字节的空间，目前总共是在栈上分配了88个字节的空间，%rsp减0x18变为了0x7fffffffdca0，然后将栈56个字节的指针存入rdx寄存器

```asm
  401463:	48 8d 4e 04          	lea    0x4(%rsi),%rcx
  401467:	48 8d 46 14          	lea    0x14(%rsi),%rax
```

lea负责取地址，然而0x4(%rsi)代表取(%rsi+4)地址的值，所以相当于把%rsi+4存入了%rcx，因为%rsi是，也就是栈帧52字节的指针。同理，%rax存的就是栈帧32字节地址，也就是%rsi+20

```asm
  40146b:	48 89 44 24 08       	mov    %rax,0x8(%rsp)
```

栈帧第-80字节存入栈帧第-20字节地址，也就是栈的-80~-73字节存的是0x7fffffffdce0。

```asm
  401470:	48 8d 46 10          	lea    0x10(%rsi),%rax
  401474:	48 89 04 24          	mov    %rax,(%rsp)
```

%rax存放%rsi+0x10，值就是0x7fffffffdcd0。栈的-88到-81字节存的就是0x7fffffffdcd0。

```asm
  401478:	4c 8d 4e 0c          	lea    0xc(%rsi),%r9
  40147c:	4c 8d 46 08          	lea    0x8(%rsi),%r8
  401480:	be c3 25 40 00       	mov    $0x4025c3,%esi
  401485:	b8 00 00 00 00       	mov    $0x0,%eax
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	callq  40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	retq   
```

%r9存栈44字节指针，%r8存栈48字节指针，%esi存0x4025c3，%eax存0，然后调用400bf0的函数，这个函数应该是会读入6个数，返回读入的数量，因为下一条指令则是判断eax是否大于5，否则爆炸。最后再把开始加的24个字节的空间放回去。现在栈有64字节空间。

```asm
0000000000400efc <phase_2>:
  400f0a:	83 3c 24 01          	cmpl   $0x1,(%rsp)
  400f0e:	74 20                	je     400f30 <phase_2+0x34>
  400f10:	e8 25 05 00 00       	callq  40143a <explode_bomb>
  400f15:	eb 19                	jmp    400f30 <phase_2+0x34>
```

现在回到了phase_2，首当其冲的就是要求栈顶元素需要等于1，否则爆炸。我输入直接蒙了六个4，然后输出发现：

```gdb
(gdb) x/gx $rsp
0x7fffffffdc20: 0x0000000400000004
```

这不就对了吗，然后试一下就发现了，当前rsp值的两个数字分别是第二个和第一个输入，又因为cmpl是比较的4个字节。所以第一个输入数字是1就行了，这里就过了。继续尝试一下，发现输入的六个数字全部存入了栈中，也是按这个顺序的。

```asm
  400f30:	48 8d 5c 24 04       	lea    0x4(%rsp),%rbx
  400f35:	48 8d 6c 24 18       	lea    0x18(%rsp),%rbp
  400f3a:	eb db                	jmp    400f17 <phase_2+0x1b>
```

rbx存入栈顶+4指针，值的地址的值就是第二个参数，rbp存入栈顶加24指针

```asm
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
```
eax存的rsp+4-4，也就是rsp，用eax相同的32位去读取，eax存下了1。然后加到了2，这里需要rbx的值和它相同，第二个参数是2就行了。

```asm
  400f25:	48 83 c3 04          	add    $0x4,%rbx
  400f29:	48 39 eb             	cmp    %rbp,%rbx
  400f2c:	75 e9                	jne    400f17 <phase_2+0x1b>
```

rbx继续加4，也就是第三个输入了。rbp在上面定义了，是栈顶+18的指针，0x7fff...很大，我尝试把3、4参数变很大，但是直接爆炸了，所以只能老实跳转到400f17。

```asm
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
```
再来一次，eax存2，那就是4，猜一下就知道了，后面一直重复，输入的参数就会是1 2 4 8 16 32，就过了。