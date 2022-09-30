#include "kernel/types.h"
#include "user.h"

void primes(int fd)
{
    int num1, num2;
    if (read(fd, &num1, sizeof(int)) != 0)
    {
        printf("prime %d\n", num1);
        int p[2];
        pipe(p); // 创建1个新管道传输数据给子进程
        int pid = fork();
        if (pid > 0)
        {
            /* parent */
            close(p[0]); // 不需要的文件描述符立即关闭
            while (read(fd, &num2, sizeof(int)) != 0)
            {
                if ((num2 % num1 != 0))
                {
                    write(p[1], &num2, sizeof(int));
                }
            }
            close(fd);
            close(p[1]);
            wait(0); // 确保子进程先退出，父进程再退出
        }
        else if (pid == 0)
        {
            /* child */
            close(p[1]); // 不需要的文件描述符立即关闭
            primes(p[0]);
        }
        else
        {
            printf("fork error\n");
            exit(-1);
        }
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Primes doesn't need arguments!\n"); // primes不需要额外输入参数
        exit(-1);
    }

    int p[2];
    pipe(p);
    int pid = fork();
    if (pid > 0)
    {
        /* parent */
        close(p[0]); // 不需要的文件描述符立即关闭
        int i;
        for (i = 2; i <= 35; i++)
        {
            write(p[1], &i, sizeof(int));
        }
        close(p[1]);
        wait(0); // 确保子进程先退出，父进程再退出
    }
    else if (pid == 0)
    {
        /* child */
        close(p[1]); // 不需要的文件描述符立即关闭
        primes(p[0]);
    }
    else
    {
        printf("fork error\n");
        exit(-1);
    }
    exit(0);
}