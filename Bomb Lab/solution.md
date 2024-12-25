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

调用了read_six_numbers函数

```asm
  ......
  40148a:	e8 61 f7 ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  40148f:	83 f8 05             	cmp    $0x5,%eax
  401492:	7f 05                	jg     401499 <read_six_numbers+0x3d>
  401494:	e8 a1 ff ff ff       	callq  40143a <explode_bomb>
  401499:	48 83 c4 18          	add    $0x18,%rsp
  40149d:	c3                   	retq   
```

注意，这个不用怎么看，血的教训。ld-linux-x86-64.so.2 是一个系统文件，它是 Linux 系统中的动态链接器（Dynamic Linker）。它的作用是加载和链接程序运行所需的共享库。动态链接器负责在程序启动时加载这些共享库，并解决程序中的外部函数或变量的引用（例如 printf、strcmp 等）。与解决炸弹任务无关。

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

rbx继续加4，也就是第三个输入了。rbp在上面定义了，是栈顶+18的指针，rbx目前还是栈顶+8，还需要循环几次才行，所以跳转到400f17。

```asm
  400f17:	8b 43 fc             	mov    -0x4(%rbx),%eax
  400f1a:	01 c0                	add    %eax,%eax
  400f1c:	39 03                	cmp    %eax,(%rbx)
  400f1e:	74 05                	je     400f25 <phase_2+0x29>
  400f20:	e8 15 05 00 00       	callq  40143a <explode_bomb>
```
再来一次，eax存2，那就是4，猜一下就知道了，后面一直重复，输入的参数就会是1 2 4 8 16 32，就过了。


## bomb3:

现在看就比较快了

```asm
0000000000400f43 <phase_3>:
  400f43:	48 83 ec 18          	sub    $0x18,%rsp
  400f47:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  400f4c:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  400f51:	be cf 25 40 00       	mov    $0x4025cf,%esi
  400f56:	b8 00 00 00 00       	mov    $0x0,%eax
  400f5b:	e8 90 fc ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  400f60:	83 f8 01             	cmp    $0x1,%eax
  400f63:	7f 05                	jg     400f6a <phase_3+0x27>
  400f65:	e8 d0 04 00 00       	callq  40143a <explode_bomb>
```

如果输入参数数量小于等于1，就爆炸。打印发现，rsp+8开始的32个字节是第一个输入，rsp+12是第二个输入。

```asm
  400f6a:	83 7c 24 08 07       	cmpl   $0x7,0x8(%rsp)
  400f6f:	77 3c                	ja     400fad <phase_3+0x6a>
  400f71:	8b 44 24 08          	mov    0x8(%rsp),%eax
  400f75:	ff 24 c5 70 24 40 00 	jmpq   *0x402470(,%rax,8)
  400f7c:	b8 cf 00 00 00       	mov    $0xcf,%eax
  400f81:	eb 3b                	jmp    400fbe <phase_3+0x7b>
```

如果7大于第一个输入，那就跳转然后爆炸，所以第一个输入大于等于7。rax是第一个输入，后面就是要跳去0x402470+2×第一个输入的位置，如果第一个输入是7，那就跳到0x400fa6，如果输入是不合法的2，那就是400f83，所以可以猜出来，2 3 4 5 6 7会依次向下排列，又因为需要大于等于7，所以必须是7。

```asm
  400f83:	b8 c3 02 00 00       	mov    $0x2c3,%eax
  400f88:	eb 34                	jmp    400fbe <phase_3+0x7b>
  400f8a:	b8 00 01 00 00       	mov    $0x100,%eax
  400f8f:	eb 2d                	jmp    400fbe <phase_3+0x7b>
  400f91:	b8 85 01 00 00       	mov    $0x185,%eax
  400f96:	eb 26                	jmp    400fbe <phase_3+0x7b>
  400f98:	b8 ce 00 00 00       	mov    $0xce,%eax
  400f9d:	eb 1f                	jmp    400fbe <phase_3+0x7b>
  400f9f:	b8 aa 02 00 00       	mov    $0x2aa,%eax
  400fa4:	eb 18                	jmp    400fbe <phase_3+0x7b>
  400fa6:	b8 47 01 00 00       	mov    $0x147,%eax
  400fab:	eb 11                	jmp    400fbe <phase_3+0x7b>

  400fad:	e8 88 04 00 00       	callq  40143a <explode_bomb>
  400fb2:	b8 00 00 00 00       	mov    $0x0,%eax
  400fb7:	eb 05                	jmp    400fbe <phase_3+0x7b>
  400fb9:	b8 37 01 00 00       	mov    $0x137,%eax
  400fbe:	3b 44 24 0c          	cmp    0xc(%rsp),%eax
  400fc2:	74 05                	je     400fc9 <phase_3+0x86>
  400fc4:	e8 71 04 00 00       	callq  40143a <explode_bomb>
  400fc9:	48 83 c4 18          	add    $0x18,%rsp
  400fcd:	c3                   	retq   
```

从0x400fa6开始，把0x147放入eax寄存器，然后调用400fbe。如果它和第二个参数一样，那就结束了，0x147是十进制的327，输入就拆除第三个炸弹了。

## bomb4：

```asm
000000000040100c <phase_4>:
  40100c:	48 83 ec 18          	sub    $0x18,%rsp
  401010:	48 8d 4c 24 0c       	lea    0xc(%rsp),%rcx
  401015:	48 8d 54 24 08       	lea    0x8(%rsp),%rdx
  40101a:	be cf 25 40 00       	mov    $0x4025cf,%esi
  40101f:	b8 00 00 00 00       	mov    $0x0,%eax
  401024:	e8 c7 fb ff ff       	callq  400bf0 <__isoc99_sscanf@plt>
  401029:	83 f8 02             	cmp    $0x2,%eax
  40102c:	75 07                	jne    401035 <phase_4+0x29>
```

看起来还是输入两个，401029告诉我们，如果eax不等于2，就爆炸。

```asm
  40102e:	83 7c 24 08 0e       	cmpl   $0xe,0x8(%rsp)
  401033:	76 05                	jbe    40103a <phase_4+0x2e>
  401035:	e8 00 04 00 00       	callq  40143a <explode_bomb>
  40103a:	ba 0e 00 00 00       	mov    $0xe,%edx
  40103f:	be 00 00 00 00       	mov    $0x0,%esi
  401044:	8b 7c 24 08          	mov    0x8(%rsp),%edi
  401048:	e8 81 ff ff ff       	callq  400fce <func4>
```

如果第一个输入大于等于14，就爆炸，所以第一个输入小于14。然后edx存入14，esi存入0，edi存入第一个输入，调用func4。不过不急着看func4

```asm
  40104d:	85 c0                	test   %eax,%eax
  40104f:	75 07                	jne    401058 <phase_4+0x4c>
  401051:	83 7c 24 0c 00       	cmpl   $0x0,0xc(%rsp)
  401056:	74 05                	je     40105d <phase_4+0x51>
  401058:	e8 dd 03 00 00       	callq  40143a <explode_bomb>
  40105d:	48 83 c4 18          	add    $0x18,%rsp
  401061:	c3                   	retq   
```

经过func4后，我们需要保证eax等于0，否则爆炸（jne代表jump if not equal）。第二个参数需要是0才能到最后。所以关键是在func4返回的时候保证eax大于0，我们来看func4

```asm
0000000000400fce <func4>:
  400fce:	48 83 ec 08          	sub    $0x8,%rsp
  400fd2:	89 d0                	mov    %edx,%eax
  400fd4:	29 f0                	sub    %esi,%eax
  400fd6:	89 c1                	mov    %eax,%ecx
  400fd8:	c1 e9 1f             	shr    $0x1f,%ecx
  400fdb:	01 c8                	add    %ecx,%eax
  400fdd:	d1 f8                	sar    %eax
  400fdf:	8d 0c 30             	lea    (%rax,%rsi,1),%ecx
```

开始就是eax = edx (eax=14); eax -= esi (eax=14); ecx = eax (ecx=14 1110); ecx右移五位=0，eax += ecx (eax=14); eax再右移一位，变成7。ecx = rax + rsi (ecx=7);

```asm
  400fe2:	39 f9                	cmp    %edi,%ecx
  400fe4:	7e 0c                	jle    400ff2 <func4+0x24>
  400ff2:	b8 00 00 00 00       	mov    $0x0,%eax
  400ff7:	39 f9                	cmp    %edi,%ecx
  400ff9:	7d 0c                	jge    401007 <func4+0x39>

  400ffb:	8d 71 01             	lea    0x1(%rcx),%esi
  400ffe:	e8 cb ff ff ff       	callq  400fce <func4>
  401003:	8d 44 00 01          	lea    0x1(%rax,%rax,1),%eax

  401007:	48 83 c4 08          	add    $0x8,%rsp
  40100b:	c3                   	retq   
```

如果第一个输入小于等于7，就跳到400ff2，eax变为0，如果第一个输入大于等于7，就返回了。我们需要保证eax=0，所以就这样就行了。第一个参数是7，第二个参数是0。

