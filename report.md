# Lab2: BombLab 实验报告

> 课程：ICS（2026 Fall） 

---

# 第一部分　推理过程


## 第 1 关：函数调用、参数传递、返回值

```
0000000000401fe0 <p1_compute>:
  401feb:	shr    $0x20,%rax              # hi  = challenge >> 32
  401ff4:	shr    $0x10,%rsi              # mid = challenge >> 16（lo = 低32位）
  401ffb:	call   401fc0 <p1_step1>
  402006:	call   401fb0 <p1_step0>
  40200f:	call   401fd0 <p1_step2>

0000000000401fc0 <p1_step1>:
  401fc4:	lea    (%rdi,%rdi,4),%eax      # hi*5
  401fc7:	add    %esi,%eax               # + mid
  401fc9:	xor    $0xec30,%eax            # first = (hi*5+mid) ^ 0xec30

0000000000401fb0 <p1_step0>:
  401fb4:	lea    (%rdi,%rsi,8),%eax      # lo + hi*8
  401fb7:	xor    $0x7b21,%eax            # second = (lo+hi*8) ^ 0x7b21

0000000000401fd0 <p1_step2>:
  401fd4:	sub    %esi,%edi               # second - first
  401fd6:	lea    0x691b(%rdi,%rdi,2),%eax  # 答案 = (second-first)*3 + 0x691b
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `lo` | challenge 低段 | `%rdx` 低32位 | `challenge` 低 32 位 |
| `mid` | challenge 中段 | `%rsi`(>>16) | `challenge >> 16` |
| `hi` | challenge 高段 | `%rax`(>>32)→`%edi` | `challenge >> 32` |
| `first` | 中间量 | `p1_step1` 返回 `%eax`→`%ecx` | `(hi*5+mid)^0xec30` |
| `second` | 中间量 | `p1_step0` 返回 `%eax` | `(lo+hi*8)^0x7b21` |

**核心公式**

```
first  = (hi*5 + mid) ^ 0xec30
second = (lo + hi*8)  ^ 0x7b21
答案   = (second - first)*3 + 0x691b
```

**C 代码**

```c
uint32_t lo = (uint32_t)challenge;
uint32_t mid = (uint32_t)(challenge >> 16);
uint32_t hi = (uint32_t)(challenge >> 32);
uint32_t first = (hi * 5u + mid) ^ 0xec30u;
uint32_t second = (lo + hi * 8u) ^ 0x7b21u;
uint32_t values[] = {(second - first) * 3u + 0x691bu};
```

## 第 2 关：数组、循环、递推关系

```
0000000000402080 <phase_02>:
  4020f0:	shr    $0x2c,%rax             # challenge >> 44
  4020f4:	and    $0x7f,%eax             # & 127
  4020f7:	xor    $0x2,%eax              # v[0] = ((c>>44)&127) ^ 2
  4020fe:	mov    $0x16,%ecx             # 起始移位量 = 22
  402103:	mov    $0x1,%edx              # 下标 i = 1
  402108:	shr    %cl,%rax               # c >> ecx
  40210e:	and    $0xf,%eax              # nibble = & 15
  402111:	add    %edx,%eax              # + i
  402113:	xor    %esi,%eax              # ^ v[i-1]
  402119:	lea    (%rax,%rax,4),%eax     # *5  → v[i]
  402120:	add    $0x1,%rdx              # i++
  402124:	add    $0x5,%ecx              # 移位量 +5 → 17+5i
  402127:	cmp    $0x6,%rdx              # 循环 i = 1..5
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `challenge` | 本关题目 | `%r13` | 参数 |
| `values[0..5]` | 输出口令数组 | `-0x40(%rbp,%rdx,4)` | 递推产生 |
| `i` | 下标 | `%rdx`（1→5） | 循环计数 |
| 移位量 | 取 nibble 的位移 | `%ecx`（初 22，+5） | `17 + 5i` |
| `nibble` | 4 位料 | `%eax`（临时） | `(challenge >> (17+5i)) & 15` |
| `v[i-1]` | 上一项 | `%esi` | 递推前值 |

**核心公式**

```
v[0] = ((challenge>>44) & 127) ^ 2
nibble_i = (challenge >> (17+5i)) & 15
v[i] = (v[i-1] ^ (nibble_i + i)) * 5          (i = 1..5)
```

**C 代码**

```c
uint32_t values[MAX_FIELDS] = {0};
values[0] = (uint32_t)((challenge >> 44) & 127u) ^ 2u;
for (unsigned i = 1; i < 6; ++i) {
    uint32_t nibble = (uint32_t)((challenge >> (17u + 5u * i)) & 15u);
    values[i] = (values[i - 1] ^ (nibble + i)) * 5u;
}
```

## 第 3 关：switch 分支与跳转表

```
0000000000402240 <phase_03>:
  402291:	and    $0x7,%edi              # v[0] 必须 == challenge & 7
  40229f:	shr    $0x3,%rax              # challenge >> 3
  4022a3:	shr    $0x20,%rsi             # challenge >> 32
  4022a7:	xor    %eax,%esi              # x = (c>>3) ^ (c>>32)
  4022a9:	call   4021e0 <p3_dispatch>

00000000004021e0 <p3_dispatch>:
  4021e8:	cmp    $0x7,%eax
  4021ed:	notrack jmp *0x4057e0(,%rax,8)   # 按 key=c&7 查跳转表（表在 0x4057e0）

0000000000402140 <p3_case_0>:  shr$1 / xor$0xa85e / add   # (x^0xa85e)+(x>>1)
0000000000402150 <p3_case_1>:  lea 0x1798(%rdi,%rdi,4)    # 5x+0x1798
0000000000402160 <p3_case_2>:  lea (,%rdi,4) / xor$0x7b4a # 4x^0x7b4a
0000000000402180 <p3_case_3>:  lea 0x31b9b(%rdi,%rdi,4)   # 5x+0x31b9b
0000000000402190 <p3_case_4>:  lea (%rdi,%rdi,1) /xor$0x425f # 2x^0x425f
00000000004021a0 <p3_case_5>:  lea 0x466e(%rdi,%rdi,4)    # 5x+0x466e
00000000004021b0 <p3_case_6>:  lea 0x5a16(%rdi,%rdi,2)    # 3x+0x5a16
00000000004021c0 <p3_case_7>:  shr$0x2 / xor$0xb495 / add # (x^0xb495)+(x>>2)
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `key` (`values[0]`) | 分支选择 | `%eax`(dispatch)；输入在 `-0x18(%rbp)` | `challenge & 7` |
| `x` | case 输入 | `%esi`→`%edi` | `(challenge>>3) ^ (challenge>>32)` |
| `values[1]` | 输出第 2 个数 | `-0x14(%rbp)` | 按 key 的分支算式 |
| 跳转表 | key→case 映射 | `.rodata 0x4057e0` | 0,6,5,1,4,7,3,2 |

> 关键坑：`p3_case_N` 的符号名按**地址**排序，并非跳转表下标。读表 `0x4057e0`（`p3_dispatch` 内 `4021f8` 起那串 `jmp`）才能得到真正的 **key→case = 0,6,5,1,4,7,3,2**。

**核心公式**

```
v[0] = challenge & 7
x    = (challenge>>3) ^ (challenge>>32)
v[1] = switch(key){ 每个 case 一条算式 }
```

**C 代码**

```c
uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
values[0] = (uint32_t)challenge & 7u;
switch (values[0]) {
    case 0: values[1] = (x ^ 0xa85eu) + (x >> 1); break;
    case 1: values[1] = 3u * x + 0x5a16u; break;
    case 2: values[1] = 5u * x + 0x466eu; break;
    case 3: values[1] = 5u * x + 0x1798u; break;
    case 4: values[1] = (2u * x) ^ 0x425fu; break;
    case 5: values[1] = (x ^ 0xb495u) + (x >> 2); break;
    case 6: values[1] = 5u * x + 0x31b9bu; break;
    case 7: values[1] = (4u * x) ^ 0x7b4au; break;
}
```

## 第 4 关：递归调用与返回过程

```
0000000000402330 <phase_04>:
  402383:	shr    $0x20,%rax            # challenge >> 32
  402387:	shr    $0x7,%rcx             # challenge >> 7
  40238b:	and    $0x7f,%edi            # target = challenge & 127
  40238e:	mov    $0x7f,%edx            # high = 127（low = 0）
  402393:	xor    %eax,%ecx             # salt = (c>>7) ^ (c>>32)
  402395:	call   4022c0 <p4_walk>

00000000004022c0 <p4_walk>:
  4022c8:	sub    %esi,%edi
  4022ca:	shr    $1,%edi
  4022cc:	add    %esi,%edi             # middle = low + (high-low)/2
  4022ce:	cmp    %eax,%edi
  4022d0:	je     402320                # 命中 → 基准
  4022d7:	lea    (%rdi,%rcx,1),%ebx    # ebx = middle + salt（预存）
  4022e7:	call   4022c0 <p4_walk>      # 右半: target > middle
  4022ec:	lea    (%rax,%rax,4),%eax / xor %ebx / xor $0xd   # 5*右 ^(m+salt)^0x0d
  402305:	call   4022c0 <p4_walk>      # 左半: target < middle
  40230a:	lea    (%rax,%rax,4),%eax / xor %ebx / xor $0x1a  # 5*左 ^(m+salt)^0x1a
  402320:	lea    0x49(%rdi),%eax / xor %ecx                 # 基准: (middle+0x49)^salt
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `target` | 查找目标 | `%edi`（入口）/`%eax`（递归内） | `challenge & 127` |
| `low` / `high` | 递归区间 | `%esi` / `%edx` | 0 / 127 |
| `salt` | 混淆盐，递归不变 | `%ecx` | `(challenge>>7)^(challenge>>32)` |
| `middle` | 区间中点 | `%edi`（walk 内） | `low+(high-low)/2` |
| `middle+salt` | 预存量 | `%ebx` | 命中/合并时用 |
| 返回值 | 递归结果 | `%eax` | 逐层 `*5^…` |

> 左右分支末尾常数不同（左 `0x1a` / 右 `0x0d`），由命中前走过的路径累积，必须照抄递归结构。

**核心公式**

```
walk(t,lo,hi): m = lo+(hi-lo)/2
  t==m: (m+0x49)^salt
  t<m : (5*walk(t,lo,m-1)) ^ (m+salt) ^ 0x1a
  t>m : (5*walk(t,m+1,hi)) ^ (m+salt) ^ 0x0d
答案 = walk(challenge&127, 0, 127)
```

**C 代码**

```c
static uint32_t walk(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + 0x49u) ^ salt;
    if (target < middle)
        return (5u * walk(target, low, middle - 1u, salt)) ^ (middle + salt) ^ 0x1au;
    return (5u * walk(target, middle + 1u, high, salt)) ^ (middle + salt) ^ 0x0du;
}
uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
uint32_t values[] = {walk((uint32_t)challenge & 127u, 0, 127, salt)};
```

## 第 5 关：结构体布局、指针、链表遍历

```
00000000004023b0 <p5_traverse>:
  4023bd:	and    $0x7,%eax
  4023c4:	mov    0x405b20(,%rax,8),%rdx  # node = heads[c&7]（表头数组 0x405b20）
  4023d3:	shr    $0x6,%rax             # c >> 6
  4023cf:	shr    $0x20,%rdi            # c >> 32
  4023d7:	xor    %edi,%eax
  4023de:	xor    $0x24a0,%eax          # value = (c>>6)^(c>>32)^0x24a0
  4023ed:	and    $0x3,%edi             # count = 1 + ((c>>3)&3)
  # 循环体：
  4023d9:	mov    0x4(%rdx),%esi        # tag = node->tag（偏移 +4）
  4023e3:	lea    (%rax,%rax,2),%eax    # value*3
  4023e6:	add    (%rdx),%eax           # + node->value（偏移 +0）
  4023eb:	xor    %esi,%eax             # ^ tag
  4023f7:	mov    0x8(%rdx),%rdx        # node = node->next（偏移 +8）
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `node` | 当前节点指针 | `%rdx` | 初值 `heads[challenge&7]` |
| `node->value/tag/next` | 节点字段 | `(%rdx)`/`0x4(%rdx)`/`0x8(%rdx)` | 结构偏移 +0/+4/+8 |
| `count` | 遍历步数 | `%edi`(&3)/`%ecx` 计数 | `1+((challenge>>3)&3)` |
| `value` | 累加结果（输出1） | `%eax` | 初值 `(c>>6)^(c>>32)^0x24a0` |
| `tag` | 末节点 tag（输出2） | `%esi`，写回 `(%r8)` | 循环中更新 |
| `heads[]` / 节点表 | 只读数据 | `0x405b20` / `0x405ba0` | 数据段 |

> 节点结构为 `{u32 value@0; u32 tag@4; node* next@8}`。节点 2 的 `next=12` 越界，但 `count≤4` 从任何表头都走不到它去读，属永远触不到的陷阱指针，不影响正确性。

**核心公式**

```
value = (challenge>>6) ^ (challenge>>32) ^ 0x24a0      // 初值
循环 count 次: tag = node->tag; value = (3*value + node->value) ^ tag; node = node->next
答案 = {value, tag}
```

**C 代码**

```c
static const struct { uint32_t value, tag; unsigned next; } nodes[] = {
    {0x596b,0xb10,5},{0x6f75,0x643,3},{0xd1ec,0x570,12},{0x4df1,0x31e,6},
    {0x7c6e,0x90a,9},{0x23d7,0x475,2},{0xa5a8,0x052,0},{0xd8e0,0xa20,11},
    {0xb847,0x11f,7},{0x7763,0x220,8},{0xa381,0x86f,1},{0x601a,0x72c,10}
};
static const unsigned heads[] = {4, 9, 8, 7, 11, 10, 1, 3};
unsigned node = heads[challenge & 7u];
unsigned count = 1u + (unsigned)((challenge >> 3) & 3u);
uint32_t value = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32) ^ 0x24a0u;
uint32_t tag = 0;
for (unsigned i = 0; i < count; ++i) {
    tag = nodes[node].tag;
    value = (3u * value + nodes[node].value) ^ tag;
    node = nodes[node].next;
}
uint32_t values[] = {value, tag};
```

## 第 6 关：函数指针表与间接调用

```
0000000000402510 <p6_dispatch>:
  40251c:	shr    $0x6,%rdi             # c >> 6
  402523:	shr    $0x20,%rax            # c >> 32
  402527:	xor    %eax,%edi             # x = (c>>6) ^ (c>>32)
  402530:	and    $0x38,%ebx            # 第二下标字节偏移 = ((c>>3)&7)*8
  402533:	and    $0x7,%eax             # 第一下标 = c & 7
  402536:	call   *0x405dc0(,%rax,8)    # ops[c&7](x)（函数指针表 0x405dc0）
  40253d:	xor    $0x3442,%eax          # ^ 0x3442 → first
  402544:	call   *0x405dc0(%rbx)       # ops[(c>>3)&7](first)
  40254f:	add    $0x2d2b,%eax          # + 0x2d2b

0000000000402480 <p6_op_0>:  shr$7 /xor /add$0xea0b       # (x^(x>>7))+0xea0b
00000000004024a0 <p6_op_1>:  lea 0xfcc9(,%rdi,4) /^(3x)   # (4x+0xfcc9)^(3x)
00000000004024c0 <p6_op_2>:  ror $1,%eax                  # ror(x,1)
00000000004024d0 <p6_op_3>:  shl$7 /xor$0xb0c3 /add       # (x^0xb0c3)+(x<<7)
00000000004024f0 <p6_op_4>:  lea 0xab65(%rdi) /shr$7 /xor # (x+0xab65)^(x>>7)
0000000000402500 <p6_op_5>:  rol$2 /xor$0xb27f            # rol(x,2)^0xb27f
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| `challenge` | 本关题目 | `%rbx` | 参数 |
| `x` | 第一次算子输入 | `%edi` | `(challenge>>6)^(challenge>>32)` |
| 第一下标 | 选 `ops` | `%eax`(&7) | `challenge & 7` |
| 第二下标 | 选 `ops`（字节偏移） | `%ebx`(&0x38) | `((challenge>>3)&7)*8` |
| `first` | 中间量 | `%eax`→`%edi` | `ops[...](x)^0x3442` |
| `ops[]` | 函数指针表 | `0x405dc0` | `[op2,op5,op3,op3,op0,op1,op1,op4]` |

> 同第 3 关：`p6_op_N` 符号名按地址排序，需读表 `0x405dc0` 得 **ops = [op2,op5,op3,op3,op0,op1,op1,op4]**。

**核心公式**

```
x     = (challenge>>6) ^ (challenge>>32)
first = ops[challenge & 7](x) ^ 0x3442
答案  = ops[(challenge>>3) & 7](first) + 0x2d2b
```

**C 代码**

```c
static uint32_t op0(uint32_t x){ return (x ^ (x >> 7)) + 0xea0bu; }
static uint32_t op1(uint32_t x){ return (4u*x + 0xfcc9u) ^ (3u*x); }
static uint32_t op2(uint32_t x){ return (x >> 1) | (x << 31); }
static uint32_t op3(uint32_t x){ return (x ^ 0xb0c3u) + (x << 7); }
static uint32_t op4(uint32_t x){ return (x + 0xab65u) ^ (x >> 7); }
static uint32_t op5(uint32_t x){ return ((x << 2) | (x >> 30)) ^ 0xb27fu; }
static uint32_t (*const ops[])(uint32_t) = {op2, op5, op3, op3, op0, op1, op1, op4};
uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
uint32_t first = ops[challenge & 7u](x) ^ 0x3442u;
uint32_t values[] = {ops[(challenge >> 3) & 7u](first) + 0x2d2bu};
```

## Secret 关（Honor）：栈上对象与相邻字段

```
00000000004026b0 <secret_gate>:
  4026cb:	lea    -0xd0(%rbp),%r12       # 局部对象基址 = -0xd0
  4026f1:	sub    $0x24,%rax             # len - 36
  4026f5:	cmp    $0x7f,%rax / jbe       # 要求 36 <= len <= 163
  402716:	call   402690 <secret_copy>   # 整串拷进对象
  40271b:	cmpl   $0x4e45504f,-0xb0(%rbp) # 偏移+32 处必须是 "OPEN"
  40272d:	lea    -0xac(%rbp),%rdi       # 数值串从偏移+36 开始解析
  402734:	call   4025d0 <secret_phase>

00000000004025d0 <secret_phase>:
  402635:	and    $0x3,%ecx / cmp -0x2c(%rbp)  # f0 必须 == mode = context&3
  40264a:	shr    $0x20,%rsi / xor %r12d       # a = challenge ^ (challenge>>32)
  40264e:	shr    $0x20,%r8  / xor %r13d       # b = context ^ (context>>32)
  402661:	xor    %r8d,%edi                    # x = a ^ b
  402664:	call   4025c0 <secret_mix>
  402669:	xor    %r9d,%eax ... ^(context>>16) # second（比 f1）
  402674:	lea    0x713e(%r8,%rcx,1) /add(a>>7) /xor  # third = second^(b+mode+0x713e+(a>>7))

00000000004025c0 <secret_mix>:
  4025c6:	ror    $0x7,%eax / xor $0xbf87      # ror(x,7) ^ 0xbf87
```

| 变量 | 含义 | 存储位置 | 来源 |
| --- | --- | --- | --- |
| 栈对象基址 | 拷贝目标 | `-0xd0(%rbp)`（`%r12`） | `secret_copy` 目标 |
| `"OPEN"` 检查位 | 须 == `0x4e45504f` | `-0xb0(%rbp)`（偏移 +32） | 魔数校验 |
| 数值串起点 | 三个数 | `-0xac(%rbp)`（偏移 +36） | `sscanf` 解析源 |
| `challenge` / `context` | 两个输入 | `%r12` / `%r13` | 参数 |
| `mode` | 输出 f0 | `%ecx` | `context & 3` |
| `a` | 中间量 | `%esi` | `challenge^(challenge>>32)` |
| `b` | 中间量 | `%r8d` | `context^(context>>32)` |
| `x` | mix 输入 | `%edi` | `a ^ b` |
| `f0`/`f1`/`f2` | 输入三个数 | `-0x2c`/`-0x28`/`-0x24(%rbp)` | 用户输入 |

> `-0xd0` 与魔数检查位 `-0xb0` 相差 `0x20=32` 字节，数值串从 `-0xac`（偏移 +36）起，故输出需先垫 `32 个 'A' + "OPEN"`，把相邻 OPEN 字段写对，三个整数才落在正确偏移。

**核心公式**

```
second = (ror(x,7) ^ 0xbf87) ^ (mode*0xceb9) ^ (context>>16)
third  = second ^ (b + mode + 0x713e + (a>>7))
输出   = "AAAA…(32)OPEN<mode> <second> <third>"
```

**C 代码**

```c
uint32_t mode = (uint32_t)context & 3u;
uint32_t a = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
uint32_t b = (uint32_t)context ^ (uint32_t)(context >> 32);
uint32_t x = a ^ b;
uint32_t second = ((x >> 7) | (x << 25)) ^ 0xbf87u
                ^ (mode * 0xceb9u) ^ (uint32_t)(context >> 16);
uint32_t third = second ^ (b + mode + 0x713eu + (a >> 7));
snprintf(out, cap, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%u %u %u", mode, second, third);
```

---

# 第二部分　结果展示

## 2.1 非公开 seed 的单关验证

用公开列表（`tests/public.json`）之外的自定义 seed `12345 / 56789 / 0702` 对各关逐一运行 `./defuser … | ./bomb …`，均显示 `Phase N defused.`。

运行命令：

![运行命令截图](<images/Screenshot 2026-10-30 at 12.39.27.png>)

运行结果（节选，各关各 seed 均 defused）：

![非公开Seed验证输出](image.png)

逐关截图（只展示前2关）

![第一关](image-1.png)

![第二关](image-2.png)

## 2.2 公开测试 `public_test.py`

`python3 public_test.py` 对六关 + Secret 关在公开 seed 上全部 PASS：

![部分输出示例](image-3.png)

结果汇总：六关公开测试 **60/60**，Secret 公开测试 **10/10** 全部通过。

---

# 第四部分　AI 辅助说明

本次实验使用了 AI 工具辅助，用途如下：

- **代码润色**：对 `defuser.c` 的实现做可读性与风格上的整理（变量命名、注释、无符号字面量后缀等），逻辑与计算关系均由本人逆向分析得出。
- **辅助报告写作**：协助将已整理好的关键汇编、变量对应、核心公式与 C 代码排版成报告结构。
- **辅助理解汇编代码**：在阅读 `bomb.S` 时，用于帮助确认个别指令语义（如 `lea` 作乘法、`ror/rol` 循环移位、跳转表/函数指针表的间接寻址）。

说明：各关的汇编分析、恢复出的计算关系与推理过程均为本人独立完成，AI 工具仅用于辅助理解与排版，未直接将工具生成的解法作为分析结果提交。