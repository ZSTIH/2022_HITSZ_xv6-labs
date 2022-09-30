#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    // 开辟一个缓冲区以接收新的参数输入
    char buffer[256] = {0};
    char *p = buffer;
    char *new_argv[256];
    int i;

    // 首先将原有argv中的参数传递给new_argv
    // argv[0]是xargs命令，需要跳过
    for (i = 1; i < argc; i++)
    {
        new_argv[i - 1] = argv[i];
    }
    new_argv[argc] = 0;

    // 将接收到的新参数传递给new_argv
    while (read(0, p, 1) == 1)
    {
        if (p[0] == '\n')
        {
            *p = 0; // 确定输入参数字符串末尾的\0符号
            new_argv[argc - 1] = buffer;
            if ((fork() == 0))
            {
                exec(new_argv[0], new_argv); // 子进程负责执行指定的命令
                exit(0);
            }
            wait(0);
            p = buffer; // p重新指向buffer，父进程准备读入下一行输入
        }
        else
        {
            p++;
        }
    }
    exit(0);
}