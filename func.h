/* 判断字符串s是否以字符串end为结尾 */
int endsWith(char *s,char *end)
{
	//首先s必须得比end长或等长
    if(strlen(s) >= strlen(end)) {
		
		//起始指针位置是两者的长度差
        int position = strlen(s) - strlen(end);
        int i;
		
		//递归检查字符是否相同
        for(i = position; i < strlen(s) ; i++) {
			
			//只要有一个字符不同就返回0
            if(s[i] != end[i - position]) {
                return 0;
            }
        }
		
		//没有问题，返回1
        return 1;
    }
	
	//s比end短
    return 0;
}

/* 获取当前用户的Home路径 */
char* getHomeDir() {
	
	//linux中自带函数getuid()，获得uid，即当前shell用户的识别码
    uid_t uid = getuid();
	
	//linux自带函数getpwuid()可通过uid获取对应用户信息(结构体passwd)，其中的pw_dir保存的是用户的home路径
    struct passwd* pwd = getpwuid(uid);
	
	//获得不到就是有问题了
    if (!pwd) {
        fprintf(stderr,"User with %u ID is unknown.\n", uid);
        exit(EXIT_FAILURE);
    }
	
	//用户home路径
    return pwd->pw_dir;
}

/* 修改当前程序工作路径 */
int changeDir(char* to) {
	
	//见下文
    errno = 0;
	
	//系统函数chdir()，修改工作路径为字符串to
	//在Linux系统中，所有的系统函数调用如果出错，都会立即将错误码储存到变量errno中(上文)
	//这个errno变量不需要程序猿定义，调用即可
    chdir(to);
	
	//所以这里直接通过errno判断之前修改工作路径是否成功
    if(errno) {
        perror("An error occured");
        return EXIT_FAILURE;
    }
    return 0;
}

/* 释放内存 */
void Deallocate(char **array,int length)
{
    int i = 0;
	
	//遍历下free()子项
    for(i = 0; i<length; i++) {
        free(array[i]);
    }
	
	//最后在free()下主指针
    free(array);
}

/* 执行命令 */
int Launch(char** args,int background) {
	
	
	//fork()是Linux中一个很有趣的函数，含义是创建一个当前程序的另一个进程
	//这个返回值的类型pid_t(process id 进程ID),本质上就是int
	//在父进程内，这里的返回值为创建的子进程ID
	//在子进程内，这里的返回值是0
	//出错返回-1
    pid_t pid = fork();

	
	//子进程内执行这里
    if(pid == 0) {
		
		//系统函数，执行程序并传参
        execvp(args[0],args);
		
		
		//父进程内执行这里
    } else if(pid > 0) {
		
		
          if(background == 0){
                int status;
                do {
					
					//系统函数waitpid()
					//总之这里在子进程没执行完前阻塞父进程
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
           } else{
			   
			   //干掉子进程并把子进程id(pid)存入链表(head)里
                kill(pid,SIGSTOP);
                head = addProcessInList(pid,head);
           }

		   
		   //进程创建失败执行这里
    } else {
        fprintf(stderr,"Fork() Error");
        return EXIT_FAILURE;
    }
	
	
    return 0;
}

/* 分析输入(先判断有没有管道然后blabla) */
int ExecuteInput(char** args, int length,int background) {
    if(args == NULL) {
        return EXIT_FAILURE;
    }
	
    // 检测是否包含了管道并对相应的左右部分进行处理
	
	// 管道前命令
    char** leftArgs = (char**)malloc(sizeof(char*) * length); 
	
	// 管道前命令迭代器
    int left_i = 0; 
	
	// 管道后命令（如果没有管道，则不会被用到）
    char** rightArgs = (char**)malloc(sizeof(char*) * length); 
	
	// 管道后命令迭代器
    int right_i = 0;
	
	// 管道控制标志
    bool left = 1;
	
	int i = 0;
    while(args[i] != NULL) {
		
		// 控制标志开且当前字符非管道字符时，迭代将命令拷贝至管道前命令**leftArgs
        if(strcmp(args[i],"|") != 0 && left == 1) { 
            leftArgs[left_i] = (char*)malloc(sizeof(char) * (strlen(args[i]) + 1));
            strcpy(leftArgs[left_i],args[i]);
            left_i++;
			
			// 控制标志关且当前字符非管道字符时，迭代将命令拷贝至管道后命令**rightArgs
        } else if(strcmp(args[i],"|") != 0 && left == 0) { 
            rightArgs[right_i] = (char*)malloc(sizeof(char) * (strlen(args[i]) + 1));
            strcpy(rightArgs[right_i],args[i]);
            right_i++;
			
			// 当前字符是管道字符，关闭管道控制标志
        } else if(strcmp(args[i],"|") == 0) { 
            left = 0;
        }
		
		
        rightArgs[right_i] = NULL;
        leftArgs[left_i] = NULL;
        i++;
    }

    // 有管道
    if(rightArgs[0] != NULL) {
        errno = 0;
        int pipefd[2];
		
		//那就建立一个管道！
		//管道的文件描述符返回到pipefd中
        pipe(pipefd);
		
		//blablabla
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        pid_t pid;
		
		//blablabla
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

		//子进程执行这里
        if(fork() == 0) {
			
			
            dup2(pipefd[0],0);
            close(pipefd[1]);
            errno =0;
			
			//执行管道右方命令
            execvp(rightArgs[0],rightArgs);
			
			//blablabla
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }
			
            waitpid(pid,NULL,0);
			
			//二号子进程执行这里
			//顺便返回下pid
        } else if((pid=fork()) == 0) { 
		
		
            dup2(pipefd[1],1);
            close(pipefd[0]);
            errno =0;
			
			//执行管道左方命令
            execvp(leftArgs[0],leftArgs);
			
			//blablabla
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }
			
            waitpid(pid,NULL,0);
        } else {
			
            waitpid(pid,NULL,0);
        }
		
		
		
		// 无管道
    } else {
		
		//cd命令修改工作目录
        if(strcmp(args[0],"cd") == 0) {
            if(args[1] == NULL) {
                char* homeDir = getHomeDir();
                if(changeDir(homeDir) == EXIT_FAILURE) {
                    return EXIT_FAILURE;
                }
            } else {
                changeDir(args[1]);
            }
			
			//exit命令退出程序
        } else if(strcmp(args[0],"exit") == 0) {
            exit(EXIT_SUCCESS);
        } else {
			
			//其它直接的执行
			return Launch(args,background);
        }
    }
	
	//释放内存
    Deallocate(leftArgs,left_i);
    Deallocate(rightArgs,right_i);
    return 0;
}

/* 获取用户输入 */
char* ReadInput() {
    char* input;
    errno = 0;
    input = (char*)malloc(g_commandMax*sizeof(char));

    if(errno) {
        perror("An error occured");
        exit(EXIT_FAILURE);
    }

    fgets(input,g_commandMax,stdin);

	//去结尾的回车
    if(input[strlen(input)-1] == '\n')
    {
        input[strlen(input)-1] = '\0';
    }

    return input;
}

/* 分割字符串，匹配*替换成匹配到的文件 */
char** TokenizeInput(char* input,int* length) {
    
    char *token;
    char **tokens;
    errno = 0;
    tokens = (char**)malloc(g_commandMaxWords*sizeof(char*));
    int i = 0;

    if(errno) {
        perror("An error occured");
        exit(EXIT_FAILURE);
    }

	
	//使用空格分割字符串，把空格换成\0并返回分割后的字符串指针，如果没有空格，则返回NULL
	const char delimiters[] = " ";
    token = strtok(input, delimiters);

	//有空格
    while( token != NULL ) 
    {
		// 通配符处理，*用在第一个位置用来表示文件名的部分匹配
		//比如*.exe
        if(token[0] == '*') {
			
			//获得当前目录并打开
            memmove(token, token+1, strlen(token));
            char directory[256];
            getcwd(directory,sizeof(directory));
            
			//文件夹目录结构体
            DIR *dir;
            struct dirent *afile;
            errno = 0;
            dir = opendir(directory);
			
			//bla
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }

			
            while( (afile=readdir(dir)) != NULL ) {
				
				//匹配文件名
                if( endsWith(afile->d_name,token) == 1)
                {
                    tokens[i] = afile->d_name;
                    i+=1;
                }
            }
			//无通配符
        } else {
            tokens[i] = token;
            i+=1;
        }
        token = strtok(NULL, delimiters);
    }
    tokens[i] = NULL;
    *length = i;
    return tokens;
}

/* 重定向文件IO */
int Redirect(char* redirectSymbol,char* filename) {
    int out,in;
    g_out = dup(1);
    g_in = dup(0);

	// > 是写入输出到文件，如果文件存在则覆盖
    if(strcmp(redirectSymbol,">") == 0) {
        errno = 0;
        out = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        if(errno) {
            perror("An error occured");
            return EXIT_FAILURE;
        }
        dup2(out,1);
        close(out);
		
		// > 是写入输出到文件，如果文件存在则追加
    } else if(strcmp(redirectSymbol,">>") == 0) {
        errno = 0;
        out = open(filename, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        if(errno) {
            perror("An error occured");
            return EXIT_FAILURE;
        }
        dup2(out,1);
        close(out);
		
		
    } else {
        errno = 0;
        in = open(filename, O_RDONLY, S_IRUSR);
        if(errno) {
            perror("An error occured");
            return EXIT_FAILURE;
        }
        dup2(in,0);
        close(in);
    }
    return 0;
}

/* 处理分割后的字符串的特殊字符 */
char** ParseInput(char* input, int* length,int* background) {
    int numOfTokens;
    int numOfArgs = 0;
    char** args;

    char** tokens = TokenizeInput(input,&numOfTokens);
	
    int i;
    for(i=0; i<numOfTokens; i++) {
		
		//IO重定向
        if(strcmp(tokens[i],">") == 0 || strcmp(tokens[i],">>") == 0 || strcmp(tokens[i],"<") == 0) {
            Redirect(tokens[i],tokens[i+1]); 
            i++; 
			
			//取群组执行结果
        } else if(i == numOfTokens-1 && strcmp(tokens[i],"&") == 0){
            *background = 1;
			
			
        } else if (i == numOfTokens-1 && tokens[i][strlen(tokens[i])-1] == '&') {
            tokens[i][strlen(tokens[i])-1] = '\0';
            numOfArgs++;
            *background = 1;
			
			
        } else {
            numOfArgs++;
        }
    }

    if(numOfArgs == 0) {
        return NULL;
    }

    args = (char**)malloc((numOfArgs+1)*sizeof(char*));
    int j = 0;
    for(i=0; i<numOfTokens; i++) {
        if(strcmp(tokens[i],">") == 0 || strcmp(tokens[i],">>") == 0 || strcmp(tokens[i],"<") == 0) {
            i++;
        } else if(!(i == numOfTokens-1 && strcmp(tokens[i],"&") == 0)) {
            args[j] = (char*)malloc(sizeof(char) * (strlen(tokens[i]) + 1));
            strcpy(args[j],tokens[i]);
            

            j++;
        }
    }
    *length = j;
    args[j] = NULL;


    return args;
}

/* 处理进程链表 */
//判断是否挂起，挂起就kill掉
void Round_Robbin(int signal){
    int status;
	
    if(head == NULL) {
        
    } else {
		
        if(running_pid == 0) {     
            running_pid=head->id;
            kill(running_pid,SIGCONT);
        } else {
            pid_t check = waitpid(running_pid,&status,WNOHANG);
            if(check == 0) {
                kill(running_pid,SIGSTOP);
                head = addProcessInList(running_pid,head);
                head = deleteFirstProcess(head);
                running_pid = head->id;
                kill(running_pid,SIGCONT);
            } else if(check == -1) {
                printf("\nProblem with executing the process!\n");
            } else {
                printf("\nProcess with pid: %d has ended!\n",running_pid);
                head = deleteFirstProcess(head);
                running_pid = 0;
            }
        }
    }
}