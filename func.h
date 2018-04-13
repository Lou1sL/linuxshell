/* 判断字符串是否是指定结尾 */
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

/* 清理内存池 */
void Deallocate(char **array,int length)
{
    int i = 0;
    for(i = 0; i<length; i++) {
        free(array[i]);
    }
    free(array);
}

/* Δέχεται ως ορίσματα τα arguments του εκτελέσιμου καθώς και μια μεταβλητή που ορίζει αν η εκτέλεση γίνεται στο υπόβαθρο */
int Launch(char** args,int background) {
    pid_t pid = fork();

    if(pid == 0) { // Διεργασία παιδί - Εκτελεί το εξωτερικό πρόγραμμα
        execvp(args[0],args);

    } else if(pid > 0) { // Διεργασία γονέας
          if(background == 0){ // Αν δεν πρέπει να εκτελεστεί στο background
                int status;
                do {
                    waitpid(pid, &status, WUNTRACED); // Περίμενε τη διεργασία παιδί, για να μη μένουν zombies διεργασίες
                } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // Η διεργασία παιδί είτε τερματίστηκε (κανονικά ή με error) είτε "σκοτώθηκε" από signal
           } else{ // Αν εκτελείται στο background η διεργασία γονέας δεν πρέπει να κάνει wait
                kill(pid,SIGSTOP);
                head = addProcessInList(pid,head); // Η διεργασία μπαίνει στη λίστα
           }

    } else { // pid < 0 -> Error
        fprintf(stderr,"Fork() Error");
        return EXIT_FAILURE;
    }
    return 0;
}

/* 执行命令 */
int ExecuteInput(char** args, int length,int background) {
    if(args == NULL) {
        return EXIT_FAILURE;
    }
    // 检测是否包含了管道
    int i = 0;
    char** leftArgs = (char**)malloc(sizeof(char*) * length); // 管道前命令
    int left_i = 0; // H θέση που βρισκόμαστε στον πίνακα leftArgs
    char** rightArgs = (char**)malloc(sizeof(char*) * length); // 管道后命令
    int right_i = 0; // H θέση που βρισκόμαστε στον πίνακα rightArgs
    bool left = 1; // Ελέγχει αν είμαστε πριν ή μετά τη διασωλήνωση εφόσον αυτή υπάρχει
    while(args[i] != NULL) {
        if(strcmp(args[i],"|") != 0 && left == 1) { // Η αριστερή εντολή της διασωλήνωσης
            leftArgs[left_i] = (char*)malloc(sizeof(char) * (strlen(args[i]) + 1));
            strcpy(leftArgs[left_i],args[i]);
            left_i++;
        } else if(strcmp(args[i],"|") != 0 && left == 0) { // Η δεξιά εντολή της διασωλήνωσης
            rightArgs[right_i] = (char*)malloc(sizeof(char) * (strlen(args[i]) + 1));
            strcpy(rightArgs[right_i],args[i]);
            right_i++;
        } else if(strcmp(args[i],"|") == 0) { // Ελέγχει αν υπάρχει διασωλήνωση
            left = 0;
        }
        rightArgs[right_i] = NULL;
        leftArgs[left_i] = NULL;
        i++;
    }

    // Εκτέλεση εντολής
    if(rightArgs[0] != NULL) { // Αν υπάρχει διασωλήνωση
        errno = 0;
        int pipefd[2];
        pipe(pipefd);
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

        errno = 0;
        pid_t pid; // Δημιουργώ διεργασία παιδί
        if(errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }

        // Το παιδί εκτελεί το αριστερό μέρος της διασωλήνωσης
        if(fork() == 0) {
            dup2(pipefd[0],0); // Ανάγνωση από είσοδο pipe όχι από stdin
            close(pipefd[1]);
            errno =0;
            execvp(rightArgs[0],rightArgs);
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }
            waitpid(pid,NULL,0);
        } else if((pid=fork()) == 0) { // Διεργασία γονέας
            dup2(pipefd[1],1); // Εγγραφή στην έξοδο του pipe όχι στο stdout
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
    } else { // Δεν υπάρχει διασωλήνωση
        if(strcmp(args[0],"cd") == 0) { // Αλλαγή directory
            if(args[1] == NULL) { // Δεν υπάρχει path για την αλλαγή του directory
            //Επιστροφή στο Home Directory του χρήστη
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

	//执行命令的回车并不需要
    if(input[strlen(input)-1] == '\n')
    {
        input[strlen(input)-1] = '\0';
    }

    return input;
}

/* Χωρισμός input του χρήστη με βάση το κενό */
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

    token = strtok(input, delimiters); // Pointer στο τελευταίο token που βρέθηκε

    while( token != NULL ) // Όσο υπάρχουν και άλλα tokens
    {
        if(token[0] == '*') { // Συναντώ wildcard άρα πρέπει να γίνει expand
            memmove(token, token+1, strlen(token)); // Αφαιρεί το wildcard
            char directory[256];
            getcwd(directory,sizeof(directory)); // Βρίκει το current directory
            // Χρησιμοποιούνται για να προσπελάσουμε το φάκελο
            DIR *dir;
            struct dirent *afile;
            errno = 0;
            dir = opendir(directory);
            if(errno) {
                perror("An error occured");
                exit(EXIT_FAILURE);
            }

            while( (afile=readdir(dir)) != NULL ) { // Όσο υπάρχουν ακόμη αρχεία
                if( endsWith(afile->d_name,token) == 1) // Αν το όνομα του αρχείου τελειώνει όπως και το wildcard
                {
                    tokens[i] = afile->d_name; // Πρόσθεσε το αρχείο στα tokens
                    i+=1;
                }
            }
        } else { // Αν δεν υπάρχει wildcard
            tokens[i] = token;
            i+=1;
        }
        token = strtok(NULL, delimiters);
    }
    tokens[i] = NULL;
    *length = i;
    return tokens;
}

/* Ανακατεύθυνση εισόδου/εξόδου από/σε αρχείο */
int Redirect(char* redirectSymbol,char* filename) {
    int out,in;
    g_out = dup(1);
    g_in = dup(0);
    /*
    // Όλα τα αρχεία αποθηκεύονται στο directory του shell, όχι στο current directory
    char path[150];
    strcpy(path,getHomeDir());
    strcat(path,"/");
    strcat(path,filename);
    */

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
    } else { // RedirectSymbol == "<"
        errno = 0;
        in = open(filename, O_RDONLY, S_IRUSR);
        if(errno) {
            perror("An error occured");
            return EXIT_FAILURE;
        }
        dup2(in,0);
        close(in);
    }
    g_redirect = 1;
    return 0;
}

/* Ανακατεύθυνση εισόδου/εξόδου πίσω στο stdin/stdout αντίστοιχα */
void directBack() {
    dup2(g_out,1);
    dup2(g_in,0);
    g_redirect = 0;
}

/* Με δεδομένη μία λιστα από tokens επιλέγει τα arguments για την κλήση των εσωτερικών/εξωτερικών προγραμμάτων */
char** ParseInput(char* input, int* length,int* background) {
    int numOfTokens;
    int numOfArgs = 0;
    char** args;

    char** tokens = TokenizeInput(input,&numOfTokens);

    // Πέρασμα για έλεγχο ύπαρξης redirection και την απαρίθμηση των args
    int i;
    for(i=0; i<numOfTokens; i++) {
        if(strcmp(tokens[i],">") == 0 || strcmp(tokens[i],">>") == 0 || strcmp(tokens[i],"<") == 0) {
            Redirect(tokens[i],tokens[i+1]); // To tokekens[i+1] περιέχει το αρχείο που χρησιμοποιείται στο redirection
            i++; // Το όνομα του αρχείου που θα γίνει το redirection δεν πρέπει να περιλαμβάνεται στα args
        } else if(i == numOfTokens-1 && strcmp(tokens[i],"&") == 0){ // Σε περίπτωση που υπάρχει κενό ανάμεσα στο τελευταίο token και το &
            *background = 1; //Αν η διεργασία πρέπει να εκτελεστεί στο υπόβαθρο αλλάζει η τιμή της background
        } else if (i == numOfTokens-1 && tokens[i][strlen(tokens[i])-1] == '&') { // Σε περίπτωση που δεν υπάρχει κενό ανάμεσα στο τελευταίο token και το &
            tokens[i][strlen(tokens[i])-1] = '\0'; // Αφαιρώ το & από τα args
            numOfArgs++;
            *background = 1;
        } else { // Το & οι χαρακτήρες ανακατεύθυνσης δεν πρέπει να συμπεριληφθούν στα args, άρα ούτε και να καταμετρηθούν
            numOfArgs++;
        }
    }

    if(numOfArgs == 0) { // Για εντολή της μορφής "> file.txt"
        return NULL;
    }

    args = (char**)malloc((numOfArgs+1)*sizeof(char*));
    int j = 0;
    for(i=0; i<numOfTokens; i++) {
        if(strcmp(tokens[i],">") == 0 || strcmp(tokens[i],">>") == 0 || strcmp(tokens[i],"<") == 0) {
            i++; // Το όνομα του αρχείου που θα γίνει το redirection δεν πρέπει να περιλαμβάνεται στα args
        } else if(!(i == numOfTokens-1 && strcmp(tokens[i],"&") == 0)) {
            args[j] = (char*)malloc(sizeof(char) * (strlen(tokens[i]) + 1));
            strcpy(args[j],tokens[i]);
            //args[j] = tokens[i];

            j++;
        }
    }
    *length = j;
    args[j] = NULL;


    return args;
}

/* Συνάρτηση που υλοποιεί τον αλγόριθμο δρομολόγησης μνήμης RR */
void Round_Robbin(int signal){
    int status;
    if(head == NULL) {
        //printf("No process in the background!\n");
    } else {       // Αν υπάρχει έστω και μια διεργασία στη λίστα
        if(running_pid == 0) {     // Αν δεν εκτελείται καμία διεργασία στο υπόβαθρο
            running_pid=head->id;   // Η διεργασία που θα εκτελεστεί είναι η πρώτη της λίστας
            kill(running_pid,SIGCONT);
        } else {   // Αν υπάρχει ήδη διεργασία από τη λίστα που εκτελείται
            pid_t check = waitpid(running_pid,&status,WNOHANG); // Ελέγχουμε την κατάσταση της εκτελούμενης διεργασίας
            if(check == 0) { // Αν η διεργασία δεν έχει τερματίσει, αλλά έχει τελειώσει ο χρόνος που της δίνει ο RR
                kill(running_pid,SIGSTOP); // Τη σταματάμε
                head = addProcessInList(running_pid,head); // Την προσθέτουμε στο τέλος της λίστας
                head = deleteFirstProcess(head);    // Τη διαγράφουμε από την αρχή της λίστας
                running_pid = head->id; // Διαλέγουμε την επόμενη διεργασία για να συνεχίσει την εκτέλεσή της
                kill(running_pid,SIGCONT);
            } else if(check == -1) {
                printf("\nProblem with executing the process!\n");
            } else {
                printf("\nProcess with pid: %d has ended!\n",running_pid);
                head = deleteFirstProcess(head); // Όταν τελειώνει διαγράφεται από τη λίστα
                running_pid = 0; // Και η διεργασία που εκτελείται αρχικοποιείται στο 0 έτσι ώστε να αρχίσει να εκτελείται η επόμενη διεργασία
            }
        }
    }
}