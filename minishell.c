#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include<errno.h>

#define MAXLINE 1024
#define MAXARGS 128

char *search_path[MAXARGS];
int path_count = 0;
char prompt[] = "minishell> ";

// 函数声明
void eval(char *cmdline);
int parseline(char *cmdline, char **argv);
int builtin_command(char **argv);
void handle_redirection(char **argv);
void execute_command(char **argv, int bg);
void execute_pipe(char**argv,int pipe_index,int bg);
void exec_with_path(char**argv);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);


void usage(void);

typedef void sighandler_t (int);
sighandler_t* Signal(int signum,sighandler_t*handler);

int main(int argc,char**argv){

    int opt;
    char cmdline[MAXLINE];

    while((opt=getopt(argc,argv,"hp"))!=-1){
        switch (opt){
            case 'h':
                usage();
                break;

            default:
                usage();
        }
    }

    search_path[0] = "/bin";
    search_path[1] = "/usr/bin";
    path_count = 2;

    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */


    while(1){
        
	    printf("%s", prompt);
	    fflush(stdout);
	    
        
        if(fgets(cmdline,MAXLINE,stdin)==NULL){
            printf("\n");
            fflush(stdout);
            break;
        }

        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }


    exit(0);
}

void usage(void) {
    printf("minishell - A simple Unix shell\n\n");
    printf("Usage: ./minishell [-h]\n\n");
    printf("Options:\n");
    printf("  -h              Show this help message\n\n");

    printf("Built-in commands:\n");
    printf("  exit            Exit the shell\n");
    printf("  cd [dir]        Change directory\n");
    printf("                  Without argument, goes to HOME\n");
    printf("  path            Show current search path\n");
    printf("  path [dir...]   Set search path (overwrites previous)\n");
    printf("                  Example: path /bin /usr/bin /usr/local/bin\n\n");

    printf("Redirection:\n");
    printf("  >  file         Redirect stdout to file (overwrite)\n");
    printf("  <  file         Redirect stdin from file\n");
    printf("  You can combine both: cmd < infile > outfile\n");
    printf("  Example: grep hello < input.txt > output.txt\n\n");

    printf("Pipeline:\n");
    printf("  |               Pipe stdout of left command to stdin of right\n");
    printf("  Example: ls -l | grep .c\n\n");

    printf("Background execution:\n");
    printf("  &               Run command in background\n");
    printf("  Example: sleep 100 &\n\n");

    printf("Signal handling:\n");
    printf("  Ctrl+C          Terminate foreground job (shell stays alive)\n");
    printf("  Ctrl+D          Exit shell (EOF)\n");

    exit(0);
}

sighandler_t*Signal(int signum, sighandler_t*handler) {
    struct sigaction action, old_action;
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if (sigaction(signum, &action, &old_action) < 0) {
        perror("Signal error");
        exit(1);
    }
    return old_action.sa_handler;
}

void sigint_handler(int sig) {
    // 空函数，Shell 忽略 Ctrl+C，子进程继承默认行为
}

void sigtstp_handler(int sig) {
    // 空函数，Shell 忽略 Ctrl+Z，子进程继承默认行为
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void eval(char *cmdline){
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;

    strcpy(buf,cmdline);
    bg=parseline(buf,argv);

    if(argv[0]==NULL){
        return;
    }

    if(builtin_command(argv)){
        return;
    }

    int pipe_index=-1;
    for(int i=0;argv[i]!=NULL;i++){
        if(!strcmp(argv[i],"|")){
            pipe_index=i;
            break;
        }
    }

    if(pipe_index!=-1){
        execute_pipe(argv,pipe_index,bg);
    }
    else{
        pid_t pid=fork();
        if (pid == 0) {
            Signal(SIGINT, SIG_DFL);
            Signal(SIGTSTP, SIG_DFL);
            handle_redirection(argv);
            exec_with_path(argv);
            perror("execvp failed");
            exit(1);
        } else if (pid > 0) {
            if (bg) {
                printf("[%d] works in background\n", pid);
            } else {
                wait(NULL);
            }
        } else {
            perror("fork failed");
        }
    }  
}

int parseline(char *cmdline, char **argv){
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf,cmdline);
    buf[strlen(buf)-1]=' ';
    while(*buf&&(*buf==' ')){
        buf++;
    }

    argc=0;
    if(*buf=='\''){
        buf++;
        delim=strchr(buf,'\'');
    }
    else{
        delim=strchr(buf,' ');
    }

    while(delim){
        argv[argc++]=buf;
        *delim='\0';
        buf=delim+1;
        while (*buf && (*buf == ' ')) {
            buf++;
        }
	       
        if (*buf == '\'') {
	        buf++;
	        delim = strchr(buf, '\'');
        }
        else {
            delim = strchr(buf, ' ');
        }
    }

    argv[argc]=NULL;
    if(0==argc){
        return 1;
    }
    
    if((bg=(*argv[argc-1]=='&')) !=0){
        argv[--argc]=NULL;
    }

    return bg;
}

int builtin_command(char **argv){
    if(!strcmp(argv[0],"exit")){
        exit(0);
    }

    if(!strcmp(argv[0],"cd")){
        if(argv[1]==NULL){
            char *home = getenv("HOME");
            if (home == NULL) {
                fprintf(stderr, "cd: HOME not set\n");
            } else {
                chdir(home);
            }
        }
        else{
            if(chdir(argv[1])){
                perror("cd failed");
            }
        }
        return 1;
    }

    if(!strcmp(argv[0],"path")){
        if(argv[1]==NULL){
            for(int i=0;i<path_count;i++){
                printf("%s ",search_path[i]);
            }
            printf("\n");
        }
        else{
            path_count=0;
            for(int i=1;argv[i]!=NULL;i++){
                search_path[path_count++]=argv[i];
            }
        }
        return 1;
    }
    return 0;
}

void handle_redirection(char **argv){
    int i = 0;
    int in_redir_index = -1;
    int null_index=-1;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], "<") == 0) {
            in_redir_index = i;
            break;
        }
        i++;
    }

    if (in_redir_index != -1) {
        // 打开目标文件
        char *infile = argv[in_redir_index + 1];
        int fd = open(infile,O_RDONLY);
        if (fd == -1) {
            perror("open failed");
            exit(1);
        }
    // 重定向标准输出
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2 failed");
            exit(1);
        }
        close(fd);
        null_index=in_redir_index;
    }

    i = 0;
    int out_redir_index = -1;
    while (argv[i] != NULL) {
        if (strcmp(argv[i], ">") == 0) {
            out_redir_index = i;
            break;
        }
        i++;
    }

    if (out_redir_index != -1) {
        char *outfile = argv[out_redir_index + 1];
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("open for output failed");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2 for output failed");
            exit(1);
        }
        close(fd);
        argv[out_redir_index]=NULL;
    }
    if(null_index>=0){
        argv[null_index]=NULL;
    }
}

void execute_pipe(char**argv,int pipe_index,int bg){
    char* left_argv[MAXARGS];
    char* right_argv[MAXARGS];
    int i,j;
    pid_t left_pid,right_pid;
    int pipefd[2];

    for(i=0;i<pipe_index;i++){
        left_argv[i]=argv[i];
    }
    left_argv[i]=NULL;

    for(i=pipe_index+1,j=0;argv[i]!=NULL;i++,j++){
        right_argv[j]=argv[i];
    }
    right_argv[j]=NULL;

    if(pipe(pipefd)==-1){
        perror("pipe failed");
        return;
    }

    if((left_pid=fork())==0){
        dup2(pipefd[1],STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        Signal(SIGINT, SIG_DFL);
        Signal(SIGTSTP, SIG_DFL);
        exec_with_path(left_argv);
        perror("execvp left failed");
        exit(1);
    }

     if ((right_pid = fork()) == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        Signal(SIGINT, SIG_DFL);
        Signal(SIGTSTP, SIG_DFL);
        exec_with_path(right_argv);
        perror("execvp right failed");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);

    if(bg){
        printf("[1] %d\n", left_pid);
        printf("[2] %d\n", right_pid);
    }else{
        wait(NULL);
        wait(NULL);
    }
}

void exec_with_path(char**argv){
        char fullpath[256];
        for(int i=0;i<path_count;i++){
            sprintf(fullpath,"%s/%s",search_path[i],argv[0]);
            if(!access(fullpath,X_OK)){
                execv(fullpath,argv);
                perror("execv failed");
                exit(1);
            }
        }
        
        printf("%s: Command not found\n", argv[0]);
        exit(1);
}