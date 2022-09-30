#include "kernel/types.h"
#include "user.h"

int main(int argc, char *argv[])
{
    if (argc != 1)
    {
        printf("Pingpong doesn't need arguments!\n"); // pingpong不需要额外输入参数
        exit(-1);
    }
    // 父进程、子进程各写入1次，因此需要2个管道
    int pipe1[2];
    int pipe2[2];
    pipe(pipe1);
    pipe(pipe2);
    int pid;
    pid = fork();
    if (pid == 0)
    {
        /* child */
        char buffer[32] = {0};
        close(pipe1[1]);                               // 子进程先读后写，关闭pipe1的写通道
        close(pipe2[0]);                               // 子进程先读后写，关闭pipe2的读通道
        read(pipe1[0], buffer, 4);                     // 子进程通过pipe1的读通道读入ping
        close(pipe1[0]);                               // 已读出ping，可以关闭pipe1的读通道
        printf("%d: received %s\n", getpid(), buffer); // 打印子进程从管道读出的ping
        write(pipe2[1], "pong", 4);                    // 子进程通过pipe2的写通道写入pong
        close(pipe2[1]);                               // 已写入pong，可以关闭pipe2的写通道
    }
    else if (pid > 0)
    {
        /* parent */
        char buffer[32] = {0};
        close(pipe1[0]);                               // 父进程先写后读，关闭pipe1的读通道
        close(pipe2[1]);                               // 父进程先写后读，关闭pipe2的写通道
        write(pipe1[1], "ping", 4);                    // 父进程通过pipe1的写通道写入ping
        close(pipe1[1]);                               // 已写入ping，可以关闭pipe1的写通道
        wait(0);                                       // 等待子进程结束。要确保子进程先退出，父进程再退出，否则容易在子进程执行期间打印出$
        read(pipe2[0], buffer, 4);                     // 父进程通过pipe2的读通道读入pong
        close(pipe2[0]);                               // 已读出pong，可以关闭pipe2的读通道
        printf("%d: received %s\n", getpid(), buffer); // 打印父进程从管道读出的pong
    }
    else
    {
        printf("fork error\n");
        exit(-1);
    }
    exit(0); //确保进程退出
}