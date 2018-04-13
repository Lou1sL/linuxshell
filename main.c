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

	//每秒调用一次Round_Robbin函数
	signal(SIGALRM,&Round_Robbin);

    while (1) {
        // Εκτύπωση προτροπής στην κονσόλα του shell
        errno = 0;
        char cwd[g_currentDirectoryMax];
        getcwd(cwd, sizeof(cwd)); // Εύρεση του τρέχοντα φακέλου που βρίσκεται το shell
        if (errno) {
            perror("An error occured");
            exit(EXIT_FAILURE);
        }
		
		//每行命令的prefix和路径提示
        printf("MyShell:%s:$ ", cwd);
        // Watning για scroll window (αποτρέπει τη σωστή εμφάνιση της προτροπής):
        // http://askubuntu.com/questions/435528/gedit-warning-gtkscrolledwindow-is-mapped-but-visible-child-gtkscrollbar-is-not
        // Κύριο πρόγραμμα
        input = ReadInput(); // 1. Ανάγνωση εντολής χρήστη
        args = ParseInput(input, &length, &background); // 2. Επεξεργασία εντολής χρήστη
        if(ExecuteInput(args,length,background) == EXIT_FAILURE) { // 3. Εκτέλεση εντολής χρήστη
                return EXIT_FAILURE;
        }
        // Αν έχει γίνει ανακατεύθυνση σε αρχείο πρέπει να γίνει πάλι ανακατεύθυνση του output στην κονσόλα
        if(g_redirect) {

            directBack();
        }
    }

    return 0;
}