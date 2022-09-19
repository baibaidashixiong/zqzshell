#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#define ZQZSH_BUFSIZE 1024          //初始BUFSIZE 1024字符长度
#define ZQZSH_FILE_BUFSIZE 8 * 1024 //初始BUFSIZE 1024字符长度
#define ZQZSH_TOK_BUFSIZE 64
#define ZQZSH_TOK_DELIM " \t\r\n\a"
char cwd[1024];
int input_list_len;
//#include <readline/history.h>

/*
  zqzsh可用命令说明:
 */
int zqzsh_cd(char **args);
int zqzsh_easycp(char **args);
int zqzsh_pwd(char **args);
int zqzsh_pipe(char **args);
int zqzsh_echo(char **args);
int zqzsh_help(char **args);
int zqzsh_exit(char **args);

/*
  内置命令列表，后跟相应的函数。
 */
char *builtin_str[] = {
    "cd",
    "easycp",
    "pwd",
    "|",
    "echo",
    "help",
    "exit"};

int (*builtin_func[])(char **) = {
    &zqzsh_cd,
    &zqzsh_easycp,
    &zqzsh_pwd,
    &zqzsh_pipe,
    &zqzsh_echo,
    &zqzsh_help,
    &zqzsh_exit};

int zqzsh_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *); //返回命令列表长度
}

/*
  命令实现。
*/

/**
   @brief 切换文件夹命令
   @param args 输入元素列表  args[0] 为 "cd".  args[1] 为所要切换的文件夹
   @return 返回1来继续运行zqzsh
 */
int zqzsh_cd(char **args)
{
    if (args[1] == NULL)
    {
        fprintf(stderr, "zqzsh: 无参数 \"cd\"\n");
    }
    else
    {
        if (chdir(args[1]) != 0)
        {
            perror("zqzsh");
        }
    }
    return 1;
}

/**
   @brief cp命令
   @return 返回1来继续运行zqzsh
 */
int zqzsh_easycp(char **args)
{
    int rd_fd, wr_fd;                   //读文件描述符 和 写文件描述符
    char buf[ZQZSH_FILE_BUFSIZE] = {0}; //缓冲区大小为128字节
    int rd_ret = 0;
    if (args[2] == NULL)
    {
        printf("请输入源目录和目标目录\n");
        return 1;
    }
    //打开源文件
    rd_fd = open(args[1], O_RDONLY);
    if (rd_fd < 0)
    {
        printf("打开文件%s失败!\n", args[1]);
        return 1;
    }
    printf("打开源文件 %s成功, 文件描述符为 = %d\n", args[1], rd_fd);

    //打开目标文件
    wr_fd = open(args[2], O_RDWR | O_CREAT);
    if (wr_fd < 0)
    {
        printf("打开目标文件%s失败!\n", args[2]);
        return 1;
    }
    while (1)
    {
        rd_ret = read(rd_fd, buf, ZQZSH_FILE_BUFSIZE); //把rd_fd所指的文件一次传送128个字节到buffer
        if (rd_ret < 128)                              //判断数据是否读取完毕
        {
            break;
        }
        write(wr_fd, buf, rd_ret);          //把buf所指的内存传送rd_ret个字节到wr_fd中
        memset(buf, 0, ZQZSH_FILE_BUFSIZE); //重置buf缓存
    }
    write(wr_fd, buf, rd_ret); //做最后一次写入

    //关闭文件描述符
    close(wr_fd);
    close(rd_fd);
    return 1;
}

/**
   @brief pwd命令，显示当前路径，运用getcwd()函数获取当前工作路径
   @return 返回1来继续运行zqzsh
 */
int zqzsh_pwd(char **args)
{
    int bufsize = 1024;
    char *buffer = malloc(sizeof(char) * bufsize);
    if (!buffer)
    {
        printf("allocation error\n");
        exit(1);
    }
    while (1)
    {
        if (getcwd(buffer, bufsize) == NULL)
        {
            bufsize += bufsize;
            buffer = realloc(buffer, sizeof(char) * bufsize);
            if (!buffer)
            {
                printf("allocation error\n");
                exit(1);
            }
        }
        else
        {
            printf("当前工作路径为 : %s\n", buffer);
            free(buffer);
            return 1;
        }
    }
}

/**
   @brief echo命令
   @return 返回1来继续运行zqzsh
 */
int zqzsh_echo(char **args)
{
    int i;
    if (args[1] == NULL)
    {
        printf("请输入正确输出\n");
    }
    else
    {
        for (i = 1; args[i] != NULL; i++)
        {
            printf("%s ", args[i]);
        }
        printf("\n");
    }
    return 1;
}

/**
   @brief help命令
   @return 返回1来继续运行zqzsh
 */
int zqzsh_help(char **args)
{
    int i;
    printf("ZQZ's ZQZSH\n");
    printf("输入程序名和参数后按enter键。\n");
    printf("以下是内置命令:\n");

    for (i = 0; i < zqzsh_num_builtins(); i++)
    {
        printf("  %s\n", builtin_str[i]);
    }

    printf("有关其他程序的信息，请使用man命令。\n");
    return 1;
}

/**
   @brief exit命令
   @return 返回0来结束运行zqzsh
 */
int zqzsh_exit(char **args)
{
    printf("欢迎下次使用！\n");
    return 0;
}

/**
  @brief 引用外部命令，用了fork(),execvp(),waitpid()三个系统函数
  @return 返回1来继续运行zqzsh
 */
int zqzsh_launch(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0)
    {
        /*
            创建出的子进程运行的代码段，用于使用execvp()函数使用外部命令
        */
        if (execvp(args[0], args) == -1)
        {
            perror("zqzsh failure");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
        // fork失败
        perror("zqzsh failure");
    }
    else
    {
        // 当前父进程运行的代码段
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        //等待子进程结束
    }

    return 1;
}

/**
   @brief readline函数
   @return 从标准输入读取字符
 */
char *zqzsh_read_line(void)
{
    // #ifdef ZQZSH_USE_STD_GETLINE
    //     char *line = NULL;
    //     ssize_t bufsize = 0;
    //     if (getline(&line, &bufsize, stdin) == -1)
    //     {
    //         if (feof(stdin))
    //         {
    //             exit(EXIT_SUCCESS);
    //         }
    //         else
    //         {
    //             perror("zqzsh: getline\n");
    //             exit(EXIT_FAILURE);
    //         }
    //     }
    //     return line;
    // #else
    int bufsize = ZQZSH_BUFSIZE;
    int i = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    //使用动态内存存储字符串
    int c;

    if (!buffer) //检查返回值，判断内存是否分配成功
    {
        fprintf(stderr, "zqzsh: allocation error\n");
        exit(EXIT_FAILURE);
    }


    while (1)
    {
        c = getchar();
        //printf("i am readline process PID=%d\n", getpid());
        if (c == EOF || c == '\n')
        {
            buffer[i] = '\0';
            return buffer;
        }
        else
        {
            buffer[i] = c;
        }
        i++;

        // 当现有字符串数量大于bufsize时，重新分配内存大小为原始的2倍
        if (i >= bufsize)
        {
            bufsize += ZQZSH_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) //检查内存分配情况
            {
                fprintf(stderr, "zqzsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    // #endif
}

/**
   @brief 将命令与参数分离开来
   @return 以NULL结尾的字符列表
 */
char **zqzsh_split_line(char *str)
{
    int bufsize = ZQZSH_TOK_BUFSIZE, i = 0; //申请64字符单位大小的字符指针用来保存字符串数组
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "zqzsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(str, ZQZSH_TOK_DELIM); // str为字符串，ZQZSH_TOK_DELIM为分隔符，如果没有可检索的字符串，则返回一个空指针
    while (token != NULL)                 //继续获取其它子字符串
    {
        tokens[i] = token;
        i++;

        if (i >= bufsize)
        {
            bufsize += ZQZSH_TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens)
            {
                fprintf(stderr, "zqzsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token = strtok(NULL, ZQZSH_TOK_DELIM); //用于从截断位置进行继续查找
    }
    tokens[i] = NULL;
    input_list_len = i;
    return tokens;
}

int execute(char **char_list)
{
    int i;
    if (char_list[0] == NULL)
    {
        return 1;
    }
    for (i = 0; i < zqzsh_num_builtins(); i++)
    {
        if (strcmp(char_list[0], builtin_str[i]) == 0)
        {
            return (*builtin_func[i])(char_list);
        }
    }
    return zqzsh_launch(char_list); //调用进程
}

/**
    @brief 用于执行管道命令（只能实现单个管道）
    @return 返回1来继续运行zqzsh
 */
int zqzsh_pipe(char **args)
{
    int pipe_pos;
    for (pipe_pos = 0; pipe_pos < input_list_len; pipe_pos++)
    {
        if (strcmp(args[pipe_pos], builtin_str[3]) == 0)
            break;
    }
    if (args[pipe_pos] == NULL)
    {
        fprintf(stderr, "zqzsh: pipe缺少参数 \n"); // 管道命令' | '后续没有指令，参数缺失
        return 1;
    }
    int fd[2];
    if (pipe(fd) == -1)
    { //创建fd[1]读取管道
        return 0;
    }
    int result = 0;
    pid_t pid = fork();

    if (pid == -1)
    {
        result = 0;
    }
    /*
        子进程运行代码段
    */
    else if (pid == 0)
    { // 子进程执行单个命令
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO); // 将标准输出重定向到fds[1]
        char **simple_line=malloc(sizeof(char *)*pipe_pos);
        for (int i = 0; i < pipe_pos; i++)
        {
            simple_line[i] = args[i];
        }
        simple_line[pipe_pos] = NULL;
        
        if (execute(simple_line) != 1)
        {
            result = 1;
        }
        
        close(fd[1]);//疑惑：为什么在此位置和在函数前都可以
        free(simple_line);
        exit(result);
    }
    else
    { // 父进程递归执行后续命令
        int status;
        waitpid(pid, &status, 0);
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO); // 将标准输入重定向到写入端fds[0]
        close(fd[0]);
        char **simple_line=malloc(sizeof(char*)*(input_list_len-pipe_pos));
        int q=0;
        for (int i = pipe_pos+1; i < input_list_len; i++)
        {
            simple_line[q] = args[i];
            q++;
        }
        simple_line[q]=NULL;
        result = execute(simple_line);
        close(fd[0]);//疑惑同上
        free(simple_line);
    }
    return result;
}

// /**
//     @brief 用于执行输出重定向
//     @return 返回1来继续运行zqzsh
//  */
// int zqzsh_commandWithRedi(char **args) { //可能含有重定向
//     int outNum = 0;
//     char *outFile = NULL;
//     int endIdx = strlen(args); // 指令在重定向前的终止下标

//     for (int i = 0; i < strlen(args); i++) {
//         if (args[i] == '>') { // 输出重定向
//             outNum++;
//             if (i+1 < strlen(args))
//                 outFile = &args[i+1];
//             else{
//                 printf("参数缺失！\n");
//             }
//             endIdx = i;
//         }
//     }
//     /* 处理重定向 */
//     if (outNum > 1) { // 输出重定向符超过一个
//         printf("重定向文件超过一个！\n");
//         return 1;
//     }

//     int result = 1;
//     pid_t pid = fork();
//     if (pid == -1) {
//         result = 0;
//     } else if (pid == 0) {
//         /* 输入输出重定向 */
//         if (outNum == 1){
//             freopen(outFile, "w", stdout);
//         }
//         /* 执行命令 */
//         args[endIdx] = '\0';
//         char** char_list = zqzsh_split_line(args);
//         int stute = execvp(char_list[0], char_list);
//         free(char_list);
//         if (stute == -1){
//             exit(1);//子进程报错后销毁，返回父进程
//         };
//         exit(0);
//     } else {
//         int status;
//         waitpid(pid, &status, 0);
//         int err = WEXITSTATUS(status); // 读取子进程的返回码

//         if (err) {
//             printf("Error: %s\n", strerror(err));
//         }
//     }
//     return result;
// }

/**
 * 命令提示符
 */
void prompt()
{
    char shell[1000];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        strcpy(shell, "22140443 zqz sh : [");
        strcat(shell, cwd);
        strcat(shell, " ] $ ");

        printf("%s", shell);
    }
    else
        perror("getcwd() error");
}

/**
   @brief 对命令进行执行
   @return 1继续，0退出
 */
int zqzsh_execute(char **args)
{
    int i;
    // //对重定向符进行命令识别
    // for (int j = 0; j < strlen(args); j++){
    //     if (args[j] == '>'){
    //         i = zqzsh_commandWithRedi(args);
    //         return i;
    //     }
    // }
    if (args[0] == NULL) //若命令为空则返回1继续
    {
        return 1;
    }
    /*
        对管道命令进行执行
    */
    for (int pipe_i = 0; pipe_i < input_list_len; pipe_i++)
    {
        if (strcmp(args[pipe_i], builtin_str[3]) == 0)
        {
            return (*builtin_func[3])(args);
        }
    }

    for (i = 0; i < zqzsh_num_builtins(); i++)
    {
        if (strcmp(args[0], builtin_str[i]) == 0) //判断与内部命令是否匹配
        {
            return (*builtin_func[i])(args); //若匹配则执行回调函数
        }
    }

    return zqzsh_launch(args); //若与内部命令不匹配则调用外部命令
}

/**
   等待读取输入内容并加以执行
 */
void zqzsh_loop(void)
{
    char *line;
    char **args;
    int status;

    do
    {
        int s_fd_out = dup(STDOUT_FILENO);
        int s_fd_in = dup(STDIN_FILENO);
        prompt();
        line = zqzsh_read_line();
        args = zqzsh_split_line(line);
        //分析并加以执行
        status = zqzsh_execute(args);
        free(line);
        free(args);
        int n_fd_out = dup2(s_fd_out , STDOUT_FILENO);
        int n_fd_in = dup2(s_fd_in,STDIN_FILENO);
    } while (status);
}

/**
   @brief 主函数入口
   @param argc 元素计数
   @param argv 元素列表
   @return 状态
 */
int main(int argc, char **argv)
{
    printf("-----------------\n");
    printf("| 欢迎使用ZQZSH！ |\n");
    printf("-----------------\n");
    // 循环等待输入.
    zqzsh_loop();
    //退出
    return EXIT_SUCCESS;
}
