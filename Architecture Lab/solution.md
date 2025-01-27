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
   ./yas sum.ys
   ```

2. 模拟运行：
   ```bash
   ./yis sum.yo
   ```

3. 查看 `%rax` 值是否为：
   ```
   0x00a + 0x0b0 + 0xc00 = 0x0cba
   ```


## arch2

partA的第二题，和arch1很像，不过需要使用call调用自身

```asm
# sum.ys: 迭代地计算链表的元素和
# Name: [你的名字]
# ID: [你的学号]

.pos 0
# 程序入口
	irmovq Stack, %rsp		# 初始化栈指针
	irmovq ele1, %rdi		# 将链表头指针传入 %rdi
	irmovq 0, %rax			# 返回值 = 0
	call rsum_list			# 调用 sum_list 函数
	halt					# 程序结束

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


rsum_list:
	pushq %rsi

	andq %rdi, %rdi			# 判断参数是否为0了
	je end

	mrmovq (%rdi),%rsi		# long val = ls->val
	mrmovq 8(%rdi),%rdi		# ls = ls->next

	call rsum_list			# long rest = rsum_list(ls->next), %rax = rest
	addq %rsi, %rax			# rest += val
end:
	popq %rsi
	ret

# 栈空间定义
.pos 0x1000					# 栈空间从 .pos 0x1000 开始，以保证程序数据和栈数据不会冲突。
.align 8
Stack:						# 栈底地址

```


## arch3

partA的第三题

```asm
# sum.ys: 迭代地计算链表的元素和
# Name: [你的名字]
# ID: [你的学号]

.pos 0
# 程序入口
	irmovq Stack, %rsp		# 初始化栈指针
	irmovq src, %rdi		# src第一个参数
	irmovq dest, %rsi		# 同上
	irmovq 3, %rdx		    # 同上
	call copy_block			# 调用 copy_block 函数
	halt					# 程序结束


# 源内存块
src:
  .quad 0x00a
  .quad 0x0b0
  .quad 0xc00
# 目标内存块
dest:
  .quad 0x111
  .quad 0x222
  .quad 0x333


copy_block:
	irmovq 0, %rax			# long result = 0
loop:
    andq %rdx, %rdx
    jle end					# while()
    mrmovq (%rdi), %r8		# long val = *src
	irmovq 8, %r9			# %r9 = 8
    addq %r9, %rdi			# src++
	rmmovq %r8, (%rsi)		# *dest = val
	addq %r9, %rsi			# dest++
	xorq %r8, %rax			# result ^= val
	irmovq 1, %r9			# %r9 = 1
    subq %r9, %rdx			# len--
    jmp loop
end:
    ret


# 栈空间定义
.pos 0x1000					# 栈空间从 .pos 0x1000 开始，以保证程序数据和栈数据不会冲突。
.align 8
Stack:						# 栈底地址

```

## arch4

就是partB

`seq-full.hcl`这段程序是一个用 HCL（Hardware Control Language）描述的单周期 Y86-64 模拟处理器控制逻辑。以下是程序的主要部分和功能的简要解释：

---

### 1. **程序结构**
该程序定义了处理器控制信号的生成规则，以支持 Y86-64 指令集的执行，包括指令获取、解码、执行、访存和写回等阶段。程序基于硬件设计语言描述信号流，类似于硬件描述语言（如 Verilog）。

---

### 2. **Fetch 阶段**
指令获取阶段的目标是从指令内存中读取下一条指令。

- **`icode` 和 `ifun`**  
  - `icode` 是指令的操作码（opcode）。
  - `ifun` 是指令的功能码（function code）。
  - 如果指令获取时发生错误（`imem_error`），默认设置为 `INOP`（空指令）。

- **`instr_valid`**  
  检查 `icode` 是否是合法的指令代码。

- **`need_regids` 和 `need_valC`**  
  确定是否需要额外的字节读取寄存器 ID（如 `rA` 和 `rB`），或是否需要一个立即数（`valC`）。

---

### 3. **Decode 阶段**
解码阶段从指令中解析寄存器和操作数。

- **`srcA` 和 `srcB`**  
  - `srcA` 和 `srcB` 确定用于 ALU 运算的源寄存器。
  - 比如，`IRRMOVQ` 指令需要 `rA` 作为 `srcA`。

- **`dstE` 和 `dstM`**  
  - `dstE` 表示 ALU 运算结果的目标寄存器。
  - `dstM` 表示从内存读取数据的目标寄存器。

---

### 4. **Execute 阶段**
执行阶段是处理器的核心，完成具体的计算操作。

- **`aluA` 和 `aluB`**  
  - ALU 的两个输入分别由 `aluA` 和 `aluB` 提供。
  - 比如，`IRRMOVQ` 的输入 A 来自 `valA`，输入 B 默认为 0。
  - 比如，`IIRMOVQ` 的输入 A 来自 `valC`，输入 B 默认为 0。
  - ValA, B均代表寄存器，而valC代表立即数

- **`alufun`**  
  - 决定 ALU 的运算类型，通常由 `ifun` 指定，默认为加法（`ALUADD`）。

- **`set_cc`**  
  - 决定是否更新条件码（Condition Codes）。

---

### 5. **Memory 阶段**
访存阶段处理对内存的读取或写入。

- **`mem_read` 和 `mem_write`**  
  - 控制信号，指示是否需要访存操作。

- **`mem_addr` 和 `mem_data`**  
  - `mem_addr` 指定访存地址。
  - `mem_data` 指定写入内存的数据。

---

### 6. **Writeback 阶段**
将结果写回寄存器。

这一部分的具体逻辑由 `dstE` 和 `dstM` 控制，结合 ALU 的计算结果或从内存中读取的值完成写回操作。

---

### 7. **程序计数器（PC）更新**
控制程序计数器（`new_pc`）的值，决定下一条指令的位置。

- **跳转和调用**  
  - 对于 `CALL` 或跳转指令，`new_pc` 设置为目标地址 `valC`。
  - 如果是返回指令（`RET`），`new_pc` 从栈中读取 `valM`。

- **默认行为**  
  对于其他情况，`new_pc` 设置为当前指令结束后的地址 `valP`。

---

### 代码

根据上面的写就行了。

```ini
#/* $begin seq-all-hcl */
####################################################################
#  HCL Description of Control for Single Cycle Y86-64 Processor SEQ   #
#  Copyright (C) Randal E. Bryant, David R. O'Hallaron, 2010       #
####################################################################

## Your task is to implement the iaddq instruction
## The file contains a declaration of the icodes
## for iaddq (IIADDQ)
## Your job is to add the rest of the logic to make it work

####################################################################
#    C Include's.  Don't alter these                               #
####################################################################

quote '#include <stdio.h>'
quote '#include "isa.h"'
quote '#include "sim.h"'
quote 'int sim_main(int argc, char *argv[]);'
quote 'word_t gen_pc(){return 0;}'
quote 'int main(int argc, char *argv[])'
quote '  {plusmode=0;return sim_main(argc,argv);}'

####################################################################
#    Declarations.  Do not change/remove/delete any of these       #
####################################################################

##### Symbolic representation of Y86-64 Instruction Codes #############
wordsig INOP 	'I_NOP'
wordsig IHALT	'I_HALT'
wordsig IRRMOVQ	'I_RRMOVQ'
wordsig IIRMOVQ	'I_IRMOVQ'
wordsig IRMMOVQ	'I_RMMOVQ'
wordsig IMRMOVQ	'I_MRMOVQ'
wordsig IOPQ	'I_ALU'
wordsig IJXX	'I_JMP'
wordsig ICALL	'I_CALL'
wordsig IRET	'I_RET'
wordsig IPUSHQ	'I_PUSHQ'
wordsig IPOPQ	'I_POPQ'
# Instruction code for iaddq instruction
wordsig IIADDQ	'I_IADDQ'

##### Symbolic represenations of Y86-64 function codes                  #####
wordsig FNONE    'F_NONE'        # Default function code

##### Symbolic representation of Y86-64 Registers referenced explicitly #####
wordsig RRSP     'REG_RSP'    	# Stack Pointer
wordsig RNONE    'REG_NONE'   	# Special value indicating "no register"

##### ALU Functions referenced explicitly                            #####
wordsig ALUADD	'A_ADD'		# ALU should add its arguments

##### Possible instruction status values                             #####
wordsig SAOK	'STAT_AOK'	# Normal execution
wordsig SADR	'STAT_ADR'	# Invalid memory address
wordsig SINS	'STAT_INS'	# Invalid instruction
wordsig SHLT	'STAT_HLT'	# Halt instruction encountered

##### Signals that can be referenced by control logic ####################

##### Fetch stage inputs		#####
wordsig pc 'pc'				# Program counter
##### Fetch stage computations		#####
wordsig imem_icode 'imem_icode'		# icode field from instruction memory
wordsig imem_ifun  'imem_ifun' 		# ifun field from instruction memory
wordsig icode	  'icode'		# Instruction control code
wordsig ifun	  'ifun'		# Instruction function
wordsig rA	  'ra'			# rA field from instruction
wordsig rB	  'rb'			# rB field from instruction
wordsig valC	  'valc'		# Constant from instruction
wordsig valP	  'valp'		# Address of following instruction
boolsig imem_error 'imem_error'		# Error signal from instruction memory
boolsig instr_valid 'instr_valid'	# Is fetched instruction valid?

##### Decode stage computations		#####
wordsig valA	'vala'			# Value from register A port
wordsig valB	'valb'			# Value from register B port

##### Execute stage computations	#####
wordsig valE	'vale'			# Value computed by ALU
boolsig Cnd	'cond'			# Branch test

##### Memory stage computations		#####
wordsig valM	'valm'			# Value read from memory
boolsig dmem_error 'dmem_error'		# Error signal from data memory


####################################################################
#    Control Signal Definitions.                                   #
####################################################################

################ Fetch Stage     ###################################

# Determine instruction code
word icode = [
	imem_error: INOP;
	1: imem_icode;		# Default: get from instruction memory
];

# Determine instruction function
word ifun = [
	imem_error: FNONE;
	1: imem_ifun;		# Default: get from instruction memory
];

bool instr_valid = icode in 
	{ INOP, IHALT, IRRMOVQ, IIRMOVQ, IRMMOVQ, IMRMOVQ,
	       IOPQ, IJXX, ICALL, IRET, IPUSHQ, IPOPQ, IIADDQ };

# Does fetched instruction require a regid byte?
bool need_regids =
	icode in { IRRMOVQ, IOPQ, IPUSHQ, IPOPQ, 
		     IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ };

# Does fetched instruction require a constant word?
bool need_valC =
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IJXX, ICALL, IIADDQ };

################ Decode Stage    ###################################

## What register should be used as the A source?
word srcA = [
	icode in { IRRMOVQ, IRMMOVQ, IOPQ, IPUSHQ  } : rA;
	icode in { IPOPQ, IRET } : RRSP;
	1 : RNONE; # Don't need register
];

## What register should be used as the B source?
word srcB = [
	icode in { IOPQ, IRMMOVQ, IMRMOVQ, IIADDQ } : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't need register
];

## What register should be used as the E destination?
word dstE = [
	icode in { IRRMOVQ } && Cnd : rB;
	icode in { IIRMOVQ, IOPQ, IIADDQ } : rB;
	icode in { IPUSHQ, IPOPQ, ICALL, IRET } : RRSP;
	1 : RNONE;  # Don't write any register
];

## What register should be used as the M destination?
word dstM = [
	icode in { IMRMOVQ, IPOPQ } : rA;
	1 : RNONE;  # Don't write any register
];

################ Execute Stage   ###################################

## Select input A to ALU
word aluA = [
	icode in { IRRMOVQ, IOPQ } : valA;
	icode in { IIRMOVQ, IRMMOVQ, IMRMOVQ, IIADDQ } : valC;
	icode in { ICALL, IPUSHQ } : -8;
	icode in { IRET, IPOPQ } : 8;
	# Other instructions don't need ALU
];

## Select input B to ALU
word aluB = [
	icode in { IRMMOVQ, IMRMOVQ, IOPQ, ICALL, 
		      IPUSHQ, IRET, IPOPQ, IIADDQ } : valB;
	icode in { IRRMOVQ, IIRMOVQ } : 0;
	# Other instructions don't need ALU
];

## Set the ALU function
word alufun = [
	icode == IOPQ : ifun;
	1 : ALUADD;
];

## Should the condition codes be updated?
bool set_cc = icode in { IOPQ, IIADDQ };

################ Memory Stage    ###################################

## Set read control signal
bool mem_read = icode in { IMRMOVQ, IPOPQ, IRET };

## Set write control signal
bool mem_write = icode in { IRMMOVQ, IPUSHQ, ICALL };

## Select memory address
word mem_addr = [
	icode in { IRMMOVQ, IPUSHQ, ICALL, IMRMOVQ } : valE;
	icode in { IPOPQ, IRET } : valA;
	# Other instructions don't need address
];

## Select memory input data
word mem_data = [
	# Value from register
	icode in { IRMMOVQ, IPUSHQ } : valA;
	# Return PC
	icode == ICALL : valP;
	# Default: Don't write anything
];

## Determine instruction status
word Stat = [
	imem_error || dmem_error : SADR;
	!instr_valid: SINS;
	icode == IHALT : SHLT;
	1 : SAOK;
];

################ Program Counter Update ############################

## What address should instruction be fetched at

word new_pc = [
	# Call.  Use instruction constant
	icode == ICALL : valC;
	# Taken branch.  Use instruction constant
	icode == IJXX && Cnd : valC;
	# Completion of RET instruction.  Use value from stack
	icode == IRET : valM;
	# Default: Use incremented PC
	1 : valP;
];
#/* $end seq-all-hcl */

```

### 测试

- **构建新模拟器**
  ```bash
  make VERSION=full
  ```
  这将基于你的 `seq-full.hcl` 文件生成一个新的 SEQ 模拟器。

- **运行简单测试**
  使用提供的 `asumi.yo` 文件测试你的实现：
  ```bash
  ./ssim -t ../y86-code/asumi.yo
  ```
  验证结果是否符合预期。

- **运行基准测试**
  验证修改后模拟器对其他指令的正确性：
  ```bash
  (cd ../y86-code; make testssim)
  ```

- **运行回归测试**
  专门测试 `iaddq` 的实现：
  ```bash
  (cd ../ptest; make SIM=../seq/ssim TFLAGS=-i)
  ```

### **评分标准**
1. **头部注释（10 分）**
   - 是否详细描述了 `iaddq` 的计算逻辑和条件码更新。

2. **基准测试（10 分）**
   - 修改后的模拟器是否能够正确执行原有的 Y86-64 指令。

3. **iaddq 测试（15 分）**
   - 是否能够通过回归测试，验证 `iaddq` 的正确性。

---

通过的话，就是
```
./optest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 58 ISA Checks Succeed
./jtest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 96 ISA Checks Succeed
./ctest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 22 ISA Checks Succeed
./htest.pl -s ../seq/ssim -i
Simulating with ../seq/ssim
  All 756 ISA Checks Succeed
```

## arch5

首先我们还是实现一个iaddq，和上面一样。成功后也是显示那些。

我们可以直接测一下效率，什么都不改，benchmark：15.18

然后我尝试把所有的加法和减法都用iaddq带替：

```asm
##################################################################
# You can modify this portion
	# Loop header
	xorq %rax,%rax		# count = 0;
	andq %rdx,%rdx		# len <= 0?
	jle Done			# if so, goto Done:

Loop:
	mrmovq (%rdi), %r10					# read val from src...
	rmmovq %r10, (%rsi)					# ...and store it to dst
	andq %r10, %r10						# val <= 0?
	jle Npos							# if so, goto Npos:
	iaddq 1, %rax						# count++
Npos:	
	iaddq 0xffffffffffffffff, %rdx		# len--
	iaddq 8, %rdi						# src++
	iaddq 8, %rsi						# dst++
	andq %rdx,%rdx						# len > 0?
	jg Loop								# if so, goto Loop:
##################################################################
```
效率变成了：benchmark：12.70

再观察一下，发现
```asm
	mrmovq 0(%rdi), %r10				# read val from src...
	rmmovq %r10, (%rsi)					# ...and store it to dst
```
这对流水线似乎会有一点数据冒险，r10会卡一下。所以我们可以提前在这里把src dst+1

```asm
##################################################################
# You can modify this portion
	# Loop header
	xorq %rax,%rax		# count = 0;
	andq %rdx,%rdx		# len <= 0?
	jle Done			# if so, goto Done:

Loop:
	mrmovq (%rdi), %r10					# read val from src...
	iaddq 8, %rdi						# src++
	rmmovq %r10, (%rsi)					# ...and store it to dst
	iaddq 8, %rsi						# dst++
	iaddq 0xffffffffffffffff, %rdx		# len--
	andq %r10, %r10						# val <= 0?
	jle Npos							# if so, goto Npos:
	iaddq 1, %rax						# count++
Npos:
	andq %rdx,%rdx						# len > 0?
	jg Loop								# if so, goto Loop:
##################################################################
```

benchmark：11.70，有一点点提升