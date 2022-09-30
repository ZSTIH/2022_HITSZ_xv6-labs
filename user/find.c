#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 获得完整新路径名，可能是文件也可能是目录
char *fmtname(char *path, char *name)
{
    int len1 = strlen(path); // find函数的路径输入参数
    int len2 = strlen(name); // path目录下找到的name，可能来自文件也可能来自目录
    char *new_name = malloc(len1 + len2 + 2);
    memset(new_name, 0, len1 + len2 + 2);
    int i;
    for (i = 0; i < len1; i++)
    {
        new_name[i] = path[i];
    }
    new_name[len1] = '/';
    for (i = len1 + 1; i < len1 + len2 + 1; i++)
    {
        new_name[i] = name[i - len1 - 1];
    }
    return new_name; // 用字符/连接字符串find和name
}

void find(char *path, char *file_name)
{
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        return;
    }

    if (st.type == T_FILE) // 若打开path之后是文件而不是目录，报错
    {
        fprintf(2, "find: not a directory path\n");
        exit(-1);
    }
    else if (st.type == T_DIR)
    {

        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if ((de.inum == 0) || (strcmp(de.name, ".") == 0) || (strcmp(de.name, "..") == 0))
            {
                continue;
            }
            // 对应当前path下搜索到的文件或目录名
            int new_fd;
            struct stat new_st;
            char *new_path = fmtname(path, de.name);
            if ((new_fd = open(new_path, 0)) < 0)
            {
                fprintf(2, "ls: cannot open %s\n", new_path);
                return;
            }
            if (fstat(new_fd, &new_st) < 0)
            {
                fprintf(2, "ls: cannot stat %s\n", new_path);
                close(new_fd);
                return;
            }

            if (new_st.type == T_FILE)
            {
                // 如果是文件，则判断文件名是否为要查找的字符串
                if (strcmp(de.name, file_name) == 0)
                {
                    printf("%s\n", new_path); // 找到则打印该文件完整的路径名
                }
            }
            else
            {
                // 如果是目录，则递归地在下一级目录查找名称与字符串匹配的文件
                find(new_path, file_name);
            }

            close(new_fd); // 及时关闭，避免资源耗尽
        }
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        // 若只有file_name参数，默认在当前目录下查找
        find(".", argv[1]);
    }
    else if (argc == 3)
    {
        find(argv[1], argv[2]);
    }
    else
    {
        printf("usage: find [path] file_name\n");
        exit(-1);
    }
    exit(0);
}