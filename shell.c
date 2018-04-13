#include "shell.h"
/*************************************
uid_t getuid();//获得当期用户的用户id
void getcwd( char* , int ) //获取但你当前工作目录
passwd* getwuid ( uid_t id )//通过用户id获得用户信息
*************************************/

#define MAX_LEN 1024

void make_prompt ( char* prompt )
{
    char host_name[MAX_LEN];//用户信息
    char path_name[MAX_LEN];//当前工作的目录
    struct passwd* pwd = getpwuid (getuid());//通过getpwuid函数借助用户id获得用户信息
    getcwd ( path_name , MAX_LEN );//获得当前的工作目录
    if ( gethostname ( host_name , MAX_LEN ))//如果没有获得当前的用户信息，用unknown替代
        strcpy ( host_name , "unknown" );
    //如果当前工作的目录是当前用户的根目录的上层，那么直接打印
    if ( strlen ( path_name ) < strlen ( pwd->pw_dir ) || strncmp(path_name , pwd->pw_dir , strlen ( pwd->pw_dir)))
        sprintf ( prompt , "[tjullin-shell]@%s:%s:" , host_name , path_name );
    else//如果当前工作的目录是当前用户的根目录的一个子孙，那么用"~"替代当前用户的根目录
        sprintf ( prompt , "[tjullin-shell]@%s:~%s:" , host_name , path_name+strlen(pwd->pw_dir) );
    switch ( getuid() )//通过用户id判断当前用户是否为根用户
    {
        case 0:
            sprintf ( prompt+strlen(prompt) , "#" );
            break;
        default:
            sprintf ( prompt+strlen(prompt) , "$" ); 
            break;
    }
}

//以“｜”为分隔得到字符串组，用0取代｜的位置代表字符串结束，实现字符串的划分
//mode代表得到的单条指令在组中的位置，只有两端的不通过管道的传递信息
//1代表中间位置
void parse_group ( char* buf )
{
    int n = strlen ( buf ),i,j=0;
    int x = cnt_cmd;
    for ( i = 0 ; i < n ; i++ )
        if ( buf[i] == '|' || i == n-1 )
        {
            int mode = 0;
            if ( buf[i] == '|' )
            {
                buf[i] = 0;
                mode = 1;
            }
            parse_command ( buf+j , mode );
            j = i+1;
        }
    group[cnt_group].first = x;//对指令进行分组
    group[cnt_group].last = cnt_cmd;
    cnt_group++;
}

//parse_command分析单条指令
void parse_command ( char* buf , int mode )
{
    int n = strlen ( buf ),i=0,j=0;
    while ( buf[i] ==' '|| buf[i] == '\t' ) i++,j++;//去掉没有用的空格
    int id = init_cmd ();//从内存池中取出一个命令的槽
    if ( mode == 1 ) cmd[id].type |= PIPE;//如果命令位于中间位置，那么设置为属性包含PIPE属性
    char* segment[MAX_ARGS];//存储划分段的位置的指针
    int x=0;
    for ( ; i < n ; i++ )
        if ( buf[i] == ' ' || i == n-1 )
        {
            if ( buf[i] == ' ' ) buf[i] = 0;
            segment[x++] = buf+j;
            j = i+1;
            while ( buf[j] == ' ' || buf[j] == '\t' )
                j++,i++;
        }
    //分析指令，查找文件的重定向和向参数列表中添加参数
    int temp = 0;
    cmd[id].cmd = segment[0];
    cmd[id].param = malloc(sizeof (char*)*(MAX_ARGS+2) );
    if ( x > 0 )
        cmd[id].param[temp++] = segment[0];
    for ( i = 1; i < x ; i++ )
    {
        int flag = 1;
        if ( strlen(segment[i]) == 1 )
        {
            if ( segment[i][0] == '<' )
            {
               flag = 0;
               cmd[id].input = segment[i+1];
               i++;
            }
            else if ( segment[i][0] == '>' )
            {
               flag = 0;
               cmd[id].output = segment[i+1];
               i++;
            }
        }
        if ( strlen ( segment[i] ) == 2 )
        {
            if ( strcmp ( segment[i] , "<<" ) == 0 )
            {
                flag = 0;
                cmd[id].input = segment[i+1];
                i++;
            }
            else if ( strcmp ( segment[i] , ">>" ) == 0 )
            {
                flag = 0;
                cmd[id].output = segment[i+1];
                i++;
            } 
        }
        if ( flag )
        {
            cmd[id].param[temp++] = segment[i];
        }
    }
}


//遍历字符串以";"为分隔得到字符串组，用0取代;代表字符串结束，实现字符串的划分
void parse_token(char* buf)
{
    int n = strlen ( buf ),i,j=0;
    n--;
    buf[n] = 0;
    for ( i = 0 ; i < n ; i++ )
        if ( buf[i] == ';'|| i == n-1 )
        {
            if ( buf[i] == ';' )
                buf[i] = 0;
            parse_group ( buf+j );
            j = i+1;
        }
}


void run_shell()
{
    int pipe_fd[2];
    int status;
    pipe(pipe_fd);
    pid_t child1,child2;
    if ((child1=fork())!=0)//父进程
    {
        if ( (child2 = fork()) == 0 )//子进程
        {
            close ( pipe_fd[1] );
            close ( fileno ( stdin ) );
            dup2 ( pipe_fd[0] , fileno(stdin));
            close ( pipe_fd[0] );
            execvp ( cmd[1].cmd , cmd[1].param );
        }
        else 
        {
            close ( pipe_fd[0]);
            close ( pipe_fd[1]);
            waitpid ( child2 , &status , 0 );
        }
        waitpid ( child1 , &status , 0 );
    }
    else
    {
        printf ( "subshell 3cmd %d\n" , getpid() );
        close ( pipe_fd[0] );
        close ( fileno(stdout));
        dup2 ( pipe_fd[1] , fileno(stdout));

        close ( pipe_fd[1] );
        execvp ( cmd[0].cmd , cmd[0].param );
    }
}

void run_command ( int l , int x )
{
    //printf ( "%d %d\n" , l , x );
    pid_t pid;
    if ( x != l )//如果不是最后一条指令，那么继续创建新的进程
    {
        pid = fork();
        if ( pid==0 )//儿子进程跑前一条指令
            run_command ( l , x-1 );
        else waitpid ( 0 , NULL , 0 );//阻塞父亲进程等待儿子进程
    }
    //printf ( "where am i %d\n" , x );
    //根据管道重定向输入输出
    if ( x != l ) dup2 ( fd[0] , fileno(stdin) );
    if ( x != r-1 ) dup2 ( fd[1] , fileno(stdout) );
　　　　//执行命令
    execvp ( cmd[x].cmd , cmd[x].param );
}


int main(int argc,char * argv[])
{

    puts("Welcome to MyShell!");
    run_shell();
    return 0;

}
