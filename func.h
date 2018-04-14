int endsWith(char *s,char *end)
{
    if(strlen(s) >= strlen(end)) {
        int position = strlen(s) - strlen(end);
        int i;
        for(i = position; i < strlen(s) ; i++) {
            if(s[i] != end[i - position]) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

/* 获取当前用户的Home路径 */
char* getHomeDir() {
    uid_t uid = getuid();
    struct passwd* pwd = getpwuid(uid);
    if (!pwd) {
        fprintf(stderr,"User with %u ID is unknown.\n", uid);
        exit(EXIT_FAILURE);
    }
    return pwd->pw_dir;
}

/* 修改当前程序工作路径 */
int changeDir(char* to) {
    errno = 0;
    chdir(to);
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
    for(i = 0; i<length; i++) {
        free(array[i]);
    }
    free(array);
}

/* 执行命令 */
int Launch(char** args,int background) {
    pid_t pid = fork();

    if(pid == 0) {
        execvp(args[0],args);
		
    } else if(pid > 0) {
          if(background == 0){
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status));
           } else{
                kill(pid,SIGSTOP);
                head = addProcessInList(pid,head);
           }

    } else {
		
        fprintf(stderr,"Fork() Error");
        return EXIT_FAILURE;
    }
    return 0;
}

/* 分析输入 */
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
		
		// 控制标志开且当前字符非管道字符时，迭代将命令拷贝至管道前命令
        if(strcmp(args[i],"|") != 0 && left == 1) { 
            leftArgs[left_i] = (char*)malloc(sizeof(char) * (strlen(args[i]) + 1));
            strcpy(leftArgs[left_i],args[i]);
            left_i++;
			
			// 控制标志关且当前字符非管道字符时，迭代将命令拷贝至管道后命令
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
        pipe(pipefd);
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        pid_t pid;
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

        if(fork() == 0) {
            dup2(pipefd[0],0);
            close(pipefd[1]);
            errno =0;
            execvp(rightArgs[0],rightArgs);
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }
			
            waitpid(pid,NULL,0);
			
			
        } else if((pid=fork()) == 0) { 
            dup2(pipefd[1],1);
            close(pipefd[0]);
            errno =0;
            execvp(leftArgs[0],leftArgs);
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
        if(strcmp(args[0],"cd") == 0) {
            if(args[1] == NULL) {
                char* homeDir = getHomeDir();
                if(changeDir(homeDir) == EXIT_FAILURE) {
                    return EXIT_FAILURE;
                }
            } else {
                changeDir(args[1]);
            }
        } else if(strcmp(args[0],"exit") == 0) {
            exit(EXIT_SUCCESS);
        } else {

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

	//去回车
    if(input[strlen(input)-1] == '\n')
    {
        input[strlen(input)-1] = '\0';
    }

    return input;
}

/* 分割字符串 */
char** TokenizeInput(char* input,int* length) {
    const char delimiters[] = " ";
    char *token;
    char **tokens;
    errno = 0;
    tokens = (char**)malloc(g_commandMaxWords*sizeof(char*));
    int i = 0;

    if(errno) {
        perror("An error occured");
        exit(EXIT_FAILURE);
    }

    token = strtok(input, delimiters);

	//有空格
    while( token != NULL ) 
    {
		// 通配符处理
        if(token[0] == '*') { 
            memmove(token, token+1, strlen(token));
            char directory[256];
            getcwd(directory,sizeof(directory));
            
            DIR *dir;
            struct dirent *afile;
            errno = 0;
            dir = opendir(directory);
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }

            while( (afile=readdir(dir)) != NULL ) {
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

    if(strcmp(redirectSymbol,">") == 0) {
        errno = 0;
        out = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        if(errno) {
            perror("An error occured");
            return EXIT_FAILURE;
        }
        dup2(out,1);
        close(out);
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
        if(strcmp(tokens[i],">") == 0 || strcmp(tokens[i],">>") == 0 || strcmp(tokens[i],"<") == 0) {
            Redirect(tokens[i],tokens[i+1]); 
            i++; 
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