#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

typedef struct
{
    char cmd[128];
    char *args[128];
}CMD, *CPtr;

unsigned int Split_Command(char *cmd, CMD command[128]);
void Split_Arguments(CPtr command);
int Run_pipe(unsigned _cmd_cnt, CPtr command);
int Run_cmd(CMD command);


int main()
{
    char cmd[256];
    CMD command[128];
    unsigned int cmd_cnt = 0;
    while(1)
    {
        for(int i = 0; i < 128; i++)
        {
            command[i].cmd[0] = NULL;
            for(int j = 0; j < 128; j++)command[i].args[j] = NULL;
        }
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        cmd_cnt = Split_Command(cmd, command);
        if (!command[0].args[0])
            continue;
        if(cmd_cnt == 1)
            Run_cmd(command[0]);
        else
            Run_pipe(cmd_cnt - 1, command);
    }
}
unsigned int Split_Command(char *cmd, CMD command[128])
{
    int i, j;
    unsigned int cnt = 0;
    char *s;
    unsigned long len;
    for(i=0; cmd[i] != '\n'; i++);//清理结尾的换行符
    cmd[i] = '\0';
    len = strlen(cmd);
    for(i = 0,j = 0, s = cmd; cmd[i]; i++)
    {
        if(cmd[i] == '|')
        {
            cmd[i-1] = 0;
            strcpy(command[j].cmd, s);
            Split_Arguments(command + j);
            j++;
            s = cmd + i + 1;
            cnt++;
        }
    }
    strcpy(command[j].cmd, s);
    Split_Arguments(command + j);
    cnt++;
    return cnt;
}
void Split_Arguments(CPtr command)
{
    int i, j, flag = 0;
    char *s;
    unsigned long len = strlen(command->cmd);
    for(i = 0; command->cmd[i]; i++)//将所有空格刷零
    {
        if(command->cmd[i] == ' ')
            command->cmd[i] = 0;
    }
    i = 0;
    while(command->cmd[i] == 0)i++;//剔除命令之前的空格
    command->args[0] = command->cmd + i;//将命令存入args[0]
    for(s = command->cmd + i, j = 1; i < len; i++)//将参数分解
    {
        if(flag && command->cmd[i] == 0)
            continue;
        else if(flag && command->cmd[i])
        {
            flag = 0;
            command->args[j] = command->cmd + i;
            j++;
        }
        else if(command->cmd[i] == 0)
        {
            flag = 1;
        }
    }
}
int Run_cmd(CMD command)
{
    if (strcmp(command.args[0], "exit") == 0)
        exit(0);
    if (strcmp(command.args[0], "cd") == 0)
    {
        if (command.args[1])
            chdir(command.args[1]);
        return 0;
    }
    if (strcmp(command.args[0], "pwd") == 0)
    {
        char wd[4096];
        puts(getcwd(wd, 4096));
        return 0;
    }
    if(strcmp(command.args[0], "export") == 0)
    {
        char PATH_NAME[128], PATH_VALUE[128];
        int i = 0;
        strcpy(PATH_NAME, command.args[1]);
        while(PATH_NAME[i] != '=')i++;
        PATH_NAME[i] = 0;
        strcpy(PATH_VALUE, PATH_NAME + i + 1);
        if(setenv(PATH_NAME, PATH_VALUE, 1) == -1)
            printf("Error changing %s\n", PATH_NAME);
        return 0;
    }
    pid_t pid = fork();
    if(pid == 0)
    {
        execvp(command.args[0], command.args);
        return 255;
    }
    else
        wait(NULL);
}
int Run_pipe(unsigned int _cmd_cnt, CPtr command)
{
    int fd[2];//fd[1]为写端，fd[0]为读端
    pid_t pid = fork();
    if(pid < 0)
    {
        printf("PID Error!\n");
        return 0;
    }
    else if(pid == 0)
    {
        if(_cmd_cnt == 0)//是第一个命令，则直接调用Run_cmd执行
        {
            Run_cmd(command[_cmd_cnt]);
            exit(0);
        }
        if(pipe(fd) < 0)
        {
            printf("Pipe Error!\n");
            return 0;
        }
        else {
            pid_t child = fork();
            if (child < 0)
            {
                printf("Child PID Error!\n");
                exit(0);
            }
            else if (child == 0)
            {
                dup2(fd[1], fileno(stdout));
                close(fd[0]);
                close(fd[1]);
                Run_pipe(_cmd_cnt - 1, command);
                exit(0);
            }
            else {
                dup2(fd[0], fileno(stdin));
                close(fd[0]);
                close(fd[1]);
                waitpid(child, NULL, 0);//等待子进程执行结束
                Run_cmd(command[_cmd_cnt]);
                exit(0);
            }
        }
    }
    else
        wait(NULL);
    return 0;

}