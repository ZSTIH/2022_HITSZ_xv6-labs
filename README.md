# 2022_HITSZ_xv6-labs

哈尔滨工业大学（深圳）2022年秋季学期《操作系统》课程实验的 xv6-labs 部分（改编自 [MIT-6.S081-2020](https://pdos.csail.mit.edu/6.828/2020/) 的课程实验）

MIT 原版实验代码的获取：

```shell
git clone git://g.csail.mit.edu/xv6-labs-2020
```

各实验代码的实现位于其它分支下，**均已通过所要求的全部测试**（部分实验要求与 MIT 原版实验**略有不同**）。

## 实验内容简介

### LAB1：XV6与UNIX实用程序

**对应分支：** `util`

需要实现5个Unix实用程序，包括：

- 了解xv6上用户程序sleep的实现。sleep程序会等待用户指定的时间。
- 实现pingpong程序，即两个进程在管道两侧来回通信。父进程将“ping”写入管道，子进程从管道将其读出并打印`<pid>：received ping`，其中`<pid>`是子进程的进程ID。子进程从父进程收到字符串后，将“pong”写入另一个管道，然后由父进程从该管道读取并打印`<pid>：received pong`，其中`<pid>`是父进程的进程ID。
- 使用管道实现“质数筛选”，输出2~35之间的所有质数。
- 实现用户程序find，即在目录树中查找名称与字符串匹配的所有文件，输出文件的相对路径。
- 实现用户程序xargs，即从标准输入中读取行并**为每行运行一次**指定的命令，且将该行作为命令的参数提供。

### LAB2：系统调用

**对应分支：** `syscall`

- 任务一：系统调用信息打印
  - 需要在xv6加入具有跟踪功能的`trace`系统调用。它可以打印系统调用信息，来帮助在之后的实验中进行debug。
- 任务二：添加系统调用`sysinfo`
  - 需要加入一条新的系统调用`sysinfo`。该系统调用将收集xv6的一些运行信息。`sysinfo`只需要一个参数，这个参数是结构体`sysinfo`的指针。xv6内核的工作就是把这个结构体填上应有的数值。

### LAB3：锁机制的应用

**对应分支：** `lock`

- 任务一：内存分配器
  - **修改内存分配器（主要修改kernel/kalloc.c）**，使每个CPU核使用独立的内存链表，而不是现在的共享链表。
- 任务二：磁盘缓存
  - 在访问文件数据的时候，操作系统会将文件的数据放置在磁盘缓存中。磁盘缓存是不同进程之间的共享资源，因此需要通过锁确保使用的正确性。如果有多进程密集地使用文件系统，它们可能会竞争磁盘缓存的`bcache.lock`锁。目前，xv6采用单个锁管理磁盘缓存，因此需要**修改磁盘缓存块列表的管理机制（主要修改kernel/bio.c）**，使得可用多个锁管理缓存块，从而减少缓存块管理的锁争用。

### LAB4：页表

**对应分支：** `pgtbl`

- 任务一：打印页表
  - 本任务中，需要加入页表打印功能，来帮助在之后的实验中进行debug。首先需要了解页表的数据结构。然后，按层次每页打印即可（可以使用迭代的算法）。
- 任务二：独立内核页表
  - 目前，xv6的每个进程都有自己独立的*用户页表*（只包含该进程用户内存的映射，从虚拟地址0开始），但是每个进程进入内核的时候，会使用唯一的一个*全局共享内核页表*。因此，需要**将全局共享内核页表改成独立内核页表**，使得每个进程拥有自己独立的内核页表，也就是全局共享内核页表的副本。
- 任务三：简化软件模拟地址翻译
  - 目前，xv6使用`kernel/vm.c`中的`copyin()/copyinstr()`将用户地址空间的数据拷贝至内核地址空间。它们通过软件模拟翻译的方式获取用户空间地址对应的物理地址，然后进行复制。因此，需要**在独立内核页表加上用户地址空间的映射，同时将函数`copyin()/copyinstr()`中的软件模拟地址翻译改成直接访问**，使得内核能够不必花费大量时间，用软件模拟的方法一步一步遍历页表，而是直接利用硬件。