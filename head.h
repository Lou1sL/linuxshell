#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/time.h>
#include <dirent.h>

int g_commandMax = 1024;
int g_commandMaxWords = 128;
int g_currentDirectoryMax = 1024;
int g_out;
int g_in;
int running_pid=0;

struct process{
    int id;
    struct process *next
}*head=NULL;

struct process* addProcessInList(int process_id, struct process *head){

    struct process *newNode = (struct process*)malloc(sizeof(struct process));

    if(newNode == NULL) {
        fprintf(stderr, "Unable to allocate memory for new node\n");
        exit(-1);
    }

    newNode->id = process_id;
    newNode->next = NULL;

    if(head == NULL){
        head = newNode;
        return head;
    } else {
        struct process *current = head;
        while (current->next!=NULL) {
            current=current->next;
        };
        current->next=newNode;
        return head;
    }

}

struct process* deleteFirstProcess(struct process *head){

    struct process *temp;
    temp=head->next;
    free(head);
    return temp;

}

int endsWith(char *s,char *end);
char* getHomeDir();
int changeDir(char* to);
void Deallocate(char **array,int length);
int Launch(char** args,int background);
int ExecuteInput(char** args, int length,int background);
char* ReadInput();
char** TokenizeInput(char* input,int* length);
int Redirect(char* redirectSymbol,char* filename);
char** ParseInput(char* input, int* length,int* background);
void Round_Robbin(int signal);