
## bomb1:

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

## bomb2:

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

## bomb4:

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

## bomb5:

同样的，我们的输入还是在rdi里面。

```asm
0000000000401062 <phase_5>:
  401062:	53                   	push   %rbx
  401063:	48 83 ec 20          	sub    $0x20,%rsp
  401067:	48 89 fb             	mov    %rdi,%rbx
  40106a:	64 48 8b 04 25 28 00 	mov    %fs:0x28,%rax
  401071:	00 00 
  401073:	48 89 44 24 18       	mov    %rax,0x18(%rsp)
  401078:	31 c0                	xor    %eax,%eax
  40107a:	e8 9c 02 00 00       	callq  40131b <string_length>
  40107f:	83 f8 06             	cmp    $0x6,%eax
  401082:	74 4e                	je     4010d2 <phase_5+0x70>
  401084:	e8 b1 03 00 00       	callq  40143a <explode_bomb>
  ...
  4010d2:	b8 00 00 00 00       	mov    $0x0,%eax
  4010d7:	eb b2                	jmp    40108b <phase_5+0x29>
```

把输入的内容存到rbx里面。fs 寄存器是一个 16 位段寄存器，可以用以下命令查看 `fs:0x28` 的值：`x/gx $fs_base + 0x28`。意义是可以检测程序栈有没有被动过。然后看输入的长度是不是等于6。eax置零再跳转

```asm
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```
`movezbl` 将源操作数的值移动到目标寄存器的低 8 位，并将目标寄存器的高 24 位清零。也就是把输入的值指向的内容的最低一个字节存到ecx中。然后把这个最低的字节也就是cl寄存器，放入rsp的值指向的内容中，也就是放入402210地址指向的字节。再把它放入rdx寄存器。再只保留edx的最后一个字节。再让rdx的值指向的内容加上0x4024b0，放回edx，并且也只保留最低字节。

rax现在是0，rax会+1，然后rax需要=6才能过最后的jne，所以很明显这是一个需要循环六次的循环。等一下再具体分析循环。

```asm
  4010ae:	c6 44 24 16 00       	movb   $0x0,0x16(%rsp)
  4010b3:	be 5e 24 40 00       	mov    $0x40245e,%esi
  4010b8:	48 8d 7c 24 10       	lea    0x10(%rsp),%rdi
  4010bd:	e8 76 02 00 00       	callq  401338 <strings_not_equal>
  4010c2:	85 c0                	test   %eax,%eax
  4010c4:	74 13                	je     4010d9 <phase_5+0x77>
  4010c6:	e8 6f 03 00 00       	callq  40143a <explode_bomb>
```
循环结束后，esi寄存器=0x40245e，对比rsp+16，两个地址指向的字符串需要相同才能不爆炸。gdb查看前者的值是flyers，所以从rsp+16开始，每一个字节都要对应上flyers。过了这个就结束了。

```asm
  40108b:	0f b6 0c 03          	movzbl (%rbx,%rax,1),%ecx
  40108f:	88 0c 24             	mov    %cl,(%rsp)
  401092:	48 8b 14 24          	mov    (%rsp),%rdx
  401096:	83 e2 0f             	and    $0xf,%edx
  401099:	0f b6 92 b0 24 40 00 	movzbl 0x4024b0(%rdx),%edx
  4010a0:	88 54 04 10          	mov    %dl,0x10(%rsp,%rax,1)
  4010a4:	48 83 c0 01          	add    $0x1,%rax
  4010a8:	48 83 f8 06          	cmp    $0x6,%rax
  4010ac:	75 dd                	jne    40108b <phase_5+0x29>
```

再来看这个循环，rax会从0一直到5，把我们输入的值+0、1、2..5指向的内容的那个字节x放入ecx，再把4024b0加上x的值的最低字节y&0x1111放入edx，再把y放入栈中，所以我们就是要找到4024b0地址的哪里是flyers。4024b0指向的内容是：`maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?`，flyers分别是字符串的第9, 15, 14, 5, 6, 7位，又因为401096与了0x1111，所以我们输入的内容的最低四位需要分别是这六个数字。十进制大概33~127都是一些常见字符，所以我们可以让01000000加上这些数字，分别是73，79，78，69，70，71，对应IONEFG，就过了。

## bomb6:

```asm
00000000004010f4 <phase_6>:
  4010f4:	41 56                	push   %r14
  4010f6:	41 55                	push   %r13
  4010f8:	41 54                	push   %r12
  4010fa:	55                   	push   %rbp
  4010fb:	53                   	push   %rbx
  4010fc:	48 83 ec 50          	sub    $0x50,%rsp
  401100:	49 89 e5             	mov    %rsp,%r13
  401103:	48 89 e6             	mov    %rsp,%rsi
  401106:	e8 51 03 00 00       	callq  40145c <read_six_numbers>
  40110b:	49 89 e6             	mov    %rsp,%r14
  40110e:	41 bc 00 00 00 00    	mov    $0x0,%r12d
  401114:	4c 89 ed             	mov    %r13,%rbp
  401117:	41 8b 45 00          	mov    0x0(%r13),%eax
  40111b:	83 e8 01             	sub    $0x1,%eax
  40111e:	83 f8 05             	cmp    $0x5,%eax
  401121:	76 05                	jbe    401128 <phase_6+0x34>
  401123:	e8 12 03 00 00       	callq  40143a <explode_bomb>
```
照样是读取六个数，然后把第一个参数放入eax（32位），要保证eax-1之后还要小于等于5，也就是第一个输入小于等于6

```asm
  401128:	41 83 c4 01          	add    $0x1,%r12d
  40112c:	41 83 fc 06          	cmp    $0x6,%r12d
  401130:	74 21                	je     401153 <phase_6+0x5f>
```
看起来像是一个循环，r12d从1开始，循环到5。

```asm
  401132:	44 89 e3             	mov    %r12d,%ebx
  401135:	48 63 c3             	movslq %ebx,%rax
  401138:	8b 04 84             	mov    (%rsp,%rax,4),%eax
  40113b:	39 45 00             	cmp    %eax,0x0(%rbp)
  40113e:	75 05                	jne    401145 <phase_6+0x51>
  401140:	e8 f5 02 00 00       	callq  40143a <explode_bomb>
```
movslq是有符号扩展，简单的说就是rax=ebx，循环1到5。然后让eax=(%rsp+4*%rax)，大胆猜一下rax应该是 1 ~ 5 循环。并且第 2 ~ 6 个输入不能与第一个输入相同，否则爆炸。

```asm
  401145:	83 c3 01             	add    $0x1,%ebx
  401148:	83 fb 05             	cmp    $0x5,%ebx
  40114b:	7e e8                	jle    401135 <phase_6+0x41>
```
ebx+1，等于2了。跳回401135，果然是循环判断。

```asm
  40114d:	49 83 c5 04          	add    $0x4,%r13
  401151:	eb c1                	jmp    401114 <phase_6+0x20>
```
r13=rsp+4，指向第二个输入，跳回401114，看了一下，意思大概是每个都不能相同，且都要小于等于6。

```asm
  401153:	48 8d 74 24 18       	lea    0x18(%rsp),%rsi
  401158:	4c 89 f0             	mov    %r14,%rax
  40115b:	b9 07 00 00 00       	mov    $0x7,%ecx
  401160:	89 ca                	mov    %ecx,%edx
  401162:	2b 10                	sub    (%rax),%edx
  401164:	89 10                	mov    %edx,(%rax)
  401166:	48 83 c0 04          	add    $0x4,%rax
  40116a:	48 39 f0             	cmp    %rsi,%rax
  40116d:	75 f1                	jne    401160 <phase_6+0x6c>
```
rsi初始为rsp+24，rax是rsp，第i个元素=7-第i个元素，rax+=4，rax不等于rsp+24就继续。跳出时，每个输入都成了7-自己.

```asm
  40116f:	be 00 00 00 00       	mov    $0x0,%esi
  401174:	eb 21                	jmp    401197 <phase_6+0xa3>

  401183:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  401188:	48 89 54 74 20       	mov    %rdx,0x20(%rsp,%rsi,2)
  40118d:	48 83 c6 04          	add    $0x4,%rsi
  401191:	48 83 fe 18          	cmp    $0x18,%rsi
  401195:	74 14                	je     4011ab <phase_6+0xb7>
  401197:	8b 0c 34             	mov    (%rsp,%rsi,1),%ecx
  40119a:	83 f9 01             	cmp    $0x1,%ecx
  40119d:	7e e4                	jle    401183 <phase_6+0x8f>
```
esi=0,ecx=第一个输入，如果小于等于1，就进入循环，循环每一个输入：让(rsp+32+2*rsi)=0x6032d0，rsi+=4，如果rsi!=24，就跳出到4011ab。否则就让ecx=(rsp+rsi)，也就是第i个输入的值，然后如果第i个输入<=1，因为前面=7-自己，所以这里相当于第i个输入开始时=6，才回去重来，要不然就下去。

```asm
  40119f:	b8 01 00 00 00       	mov    $0x1,%eax
  4011a4:	ba d0 32 60 00       	mov    $0x6032d0,%edx
  4011a9:	eb cb                	jmp    401176 <phase_6+0x82>

  401176:	48 8b 52 08          	mov    0x8(%rdx),%rdx
  40117a:	83 c0 01             	add    $0x1,%eax
  40117d:	39 c8                	cmp    %ecx,%eax
  40117f:	75 f5                	jne    401176 <phase_6+0x82>
  401181:	eb 05                	jmp    401188 <phase_6+0x94>
```
eax=1,edx=0x6032d0。跳转后rdx=6032e0，eax=2，如果第i个输入 != eax，就继续，如果等于，就跳到上面。暂时没看懂目的是什么，似乎只改变了(rsp+32+2*rsi)的值。

不过经过检查，可以发现放入栈$rsp+0x20+2*? 改变了的值指向的内容另有玄机：
```gdb
(gdb) x/gx 0x6032e0
0x6032e0 <node2>:       0x00000002000000a8
(gdb) x/gx 0x6032d0
0x6032d0 <node1>:       0x000000010000014c
(gdb) x/gx 0x6032f0
0x6032f0 <node3>:       0x000000030000039c
(gdb) x/gx 0x603300
0x603300 <node4>:       0x00000004000002b3
(gdb) x/gx 0x603310
0x603310 <node5>:       0x00000005000001dd
(gdb) x/gx 0x603320
0x603320 <node6>:       0x00000006000001bb
```
这个是有顺序的，32位的情况下，0x6032d0代表1，然后+0x10，就+1。如果输入是1~6，这里就是6~1。

```asm
  4011ab:	48 8b 5c 24 20       	mov    0x20(%rsp),%rbx
  4011b0:	48 8d 44 24 28       	lea    0x28(%rsp),%rax
  4011b5:	48 8d 74 24 50       	lea    0x50(%rsp),%rsi
  4011ba:	48 89 d9             	mov    %rbx,%rcx
  4011bd:	48 8b 10             	mov    (%rax),%rdx
  4011c0:	48 89 51 08          	mov    %rdx,0x8(%rcx)
  4011c4:	48 83 c0 08          	add    $0x8,%rax
  4011c8:	48 39 f0             	cmp    %rsi,%rax
  4011cb:	74 05                	je     4011d2 <phase_6+0xde>
  4011cd:	48 89 d1             	mov    %rdx,%rcx
  4011d0:	eb eb                	jmp    4011bd <phase_6+0xc9>
```
rax=rsp+0x20+8, rsi=rsp+0x20+48, rcx=刚刚放进栈去的第一个内容，rdx=刚刚放进栈去的第二个内容，刚刚放进栈去的第一个内容+8指向的值（这个内容+0x10）变成刚刚放进栈去的第二个。循环六次

```asm
  4011d2:	48 c7 42 08 00 00 00 	movq   $0x0,0x8(%rdx)
  4011d9:	00 
  4011da:	bd 05 00 00 00       	mov    $0x5,%ebp
  4011df:	48 8b 43 08          	mov    0x8(%rbx),%rax
  4011e3:	8b 00                	mov    (%rax),%eax
  4011e5:	39 03                	cmp    %eax,(%rbx)
  4011e7:	7d 05                	jge    4011ee <phase_6+0xfa>
  4011e9:	e8 4c 02 00 00       	callq  40143a <explode_bomb>
```
循环6次。rbx=放进栈去的第一个内容。eax=放进栈去的第二个内容的32位版本，如果放进栈去的第一个内容指向的值小于放进栈去的第二个内容指向的值就跳4011ee，否则爆炸。

注意，cmp的比较：

立即数与寄存器比较（如 cmp $5, %eax）：

在这种情况下，比较的是寄存器的值与立即数，计算的逻辑为 eax - 5。

寄存器与内存比较（如 cmp %eax, (%rbx)）：

在这种情况下，比较的是寄存器的值与内存中值，计算的逻辑为 mem - eax。这意味着 %eax 的值是从内存值中减去的。

```asm
  4011ee:	48 8b 5b 08          	mov    0x8(%rbx),%rbx
  4011f2:	83 ed 01             	sub    $0x1,%ebp
  4011f5:	75 e8                	jne    4011df <phase_6+0xeb>
  4011f7:	48 83 c4 50          	add    $0x50,%rsp
  4011fb:	5b                   	pop    %rbx
  4011fc:	5d                   	pop    %rbp
  4011fd:	41 5c                	pop    %r12
  4011ff:	41 5d                	pop    %r13
  401201:	41 5e                	pop    %r14
  401203:	c3                   	retq   
```
rbx=(rbx+8)，这里差不多就清楚了。就是要放进栈去的第i个内容指向的值大于放进栈去的第i+1个内容，比较的是32位的

如果我输入的是2 1 6 5 4 3的话，存入栈的值

```gdb
(gdb) x/gx $rsp+32
0x7fffffffdc00: 0x0000000000603310
(gdb) x/gx $rsp+32+8
0x7fffffffdc08: 0x0000000000603320
(gdb) x/gx $rsp+32+16
0x7fffffffdc10: 0x00000000006032d0
(gdb) x/gx $rsp+32+24
0x7fffffffdc18: 0x00000000006032e0
(gdb) x/gx $rsp+32+32
0x7fffffffdc20: 0x00000000006032f0
(gdb) x/gx $rsp+32+40
0x7fffffffdc28: 0x0000000000603300
```

```gdb
(gdb) x/gx 0x6032e0-0x10
0x6032d0 <node1>:       0x000000010000014c
(gdb) x/gx 0x6032e0
0x6032e0 <node2>:       0x00000002000000a8
(gdb) x/gx 0x6032e0+0x10
0x6032f0 <node3>:       0x000000030000039c
(gdb) x/gx 0x6032e0+0x20
0x603300 <node4>:       0x00000004000002b3
(gdb) x/gx 0x6032e0+0x30
0x603310 <node5>:       0x00000005000001dd
(gdb) x/gx 0x6032e0+0x40
0x603320 <node6>:       0x00000006000001bb
```

这就有一个很明显的对应关系了，输入4 3 2 1 6 5即可满足放进栈去的第i个内容指向的值大于放进栈去的第i+1个内容。
