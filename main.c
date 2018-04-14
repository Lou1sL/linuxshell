#include "head.h"
#include "func.h"

int main()
{
    char* input;
    char** args;
    int length;

    // 把当前shell的工作路径设置为当前用户的home路径
    char* homeDir = getHomeDir();
    changeDir(homeDir);
	
    int background = 0;

    struct timeval value={1,0};
	struct timeval interval={1,0};
	struct itimerval timer={interval,value};
	setitimer(ITIMER_REAL, &timer, 0);

	//轮询
	signal(SIGALRM,&Round_Robbin);

	//主循环
    while (1) {
        
        errno = 0;
        char cwd[g_currentDirectoryMax];
		
		//当前工作路径
        getcwd(cwd, sizeof(cwd)); 
        if (errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }
		
		//每行命令的prefix和路径提示
        printf("MyShell:%s:$ ", cwd);
		
		//读输入，执行
        input = ReadInput(); 
        args = ParseInput(input, &length, &background);
        if(ExecuteInput(args,length,background) == EXIT_FAILURE) {
                return EXIT_FAILURE;
        }
    }

    return 0;
}