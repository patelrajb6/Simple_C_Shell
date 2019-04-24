#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define Max 150

// Attributions: Computer Systems: A Programmer's Perspective, Randal E. Bryant and David R. O'Hallaron, Third Edition.
//               Chapter 8.
// Functions: parseline and eval, builtin
// Attribution: Man page of linux
//Referred books :Linux System Programming, 2nd Edition by Robert Love.(good book)


int builtin_command(char **argv) //function from the book to check for built in functions
{
    if (!strcmp(argv[0], "exit")) /* quit command */
        exit(0);
    if (!strcmp(argv[0], "&")) /* Ignore singleton & */
        return 1;
    return 0; /* Not a builtin command */
}
int parseline(char* string,char** token)  //parseline function which is from the book.
{
    char* delim;
    int argc=0;
    int bg;

    string[strlen(string)-1]= ' ';
    while(*string && (*string==' '))
    {
        string++;
    }

    while ((delim = strchr(string, ' ')))
    {
        token[argc++]= string;
        *delim ='\0';
        string=delim+1;
        while (*string && (*string == ' '))
        {
            string++;
        }
    }
    token[argc]=NULL;
    if(argc==0)
       return 1;

    if((bg=(*token[argc-1]=='&'))!=0)
       token[--argc]=NULL;

    return bg;
    
}
int findredirect(char *sym, char **total) //symbol checker <,>,>> a helper function
{
    char *a = NULL;
    int i = 0;
    while (total[i] != a)
    {
        if (strcmp(total[i], sym) == 0)
        {
            return 1;
        }
        i++;
    }
    return 0;
}
char* GetFileName(char **tot, char *b) //function to get the name of the file which is being redirected
{
    char *a = NULL;
    int i = 0;
    char *argv;
    char *total[Max];
    int j = 0;
    while (tot[j] != a)
    {
        total[j] = tot[j];
        j++;
    }
    total[j + 1] = NULL;
    
    while (total[i] != a)
    {
        if (strcmp(total[i], b) == 0)
        {
            argv = total[i + 1];
            //total[i] = NULL;
            return argv;
        }
        i++;
    }

    return NULL;
}
void InputRedirect(char **total, char *file) //input redirecting function
{
   
    int fd = open(file, O_RDONLY);
    dup2(fd, 0);
}
void OutputRedirect(char *file, char *symbol)  //output redirecting function
{
    int fd;
    if ((strcmp(symbol, ">")) == 0)
    {
        fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    }
    else if ((strcmp(symbol, ">>")) == 0)
    {
        fd = open(file, O_APPEND | O_WRONLY | O_CREAT, 0644);
    }
    dup2(fd, 1);
}
int pipecheck(char **input) // function to count pipes
{
    
    char buf[Max];
    int ar[Max];      
    int numPipes = 0; 
    int n = 0;
    char *a = NULL;
    char *b = "|";
    int j = 0;
    int p = 0;
    while (*(input + n) != a)
    {
        if (strcmp(*(input + n), b) == 0)
        {
            numPipes++;
        }

        n++;
    }
    return numPipes;
}
void _SignalHandler(int num)  //a single handler which can handle all the given signals
{
    if (num == SIGINT)
    {
        write(1, "\nCaught: Interrupt\n", 19);
        
        exit(0);
    }

    else if (num == SIGTERM)
    {
        write(1, "Caught: Terminated\n", 19);
        exit(0);
    }

    else if (num == SIGCONT)
    {
        write(1, "\nCS361 > ", 7);
    }
    else if (num == SIGCHLD)
    {
        sleep(1);
        int status;
        pid_t pid;
        while ((pid = waitpid(-1, &status, 0)) > 0)
        {
            if (WIFEXITED(status))
                printf("pid:%d status:%d\n", pid, WEXITSTATUS(status));
            else
                printf("pid:%d status:%d\n", pid, WEXITSTATUS(status));
        }
    }
}
void findReplace(char **argv)
{
    int j = 0;
    char *a = NULL;
    char *b = "<";
    char *c = ">";
    char *d = ">>";
    while (argv[j] != a)
    {

        if (strcmp(argv[j], b) == 0)
        {
            argv[j] = NULL;
        }
        else if (strcmp(argv[j], c) == 0)
        {
            argv[j] = NULL;
        }
        else if (strcmp(argv[j], d) == 0)
        {
            argv[j] = NULL;
        }
        j++;
    }
}
void eval(char *cmdline,pid_t backpid,pid_t forepid)  // function which evaluates single arguments calls
{
    char *argv[Max]; /* Argument list execve() */
    char buf[Max];   /* Holds modified command line */
    int bg;          /* Should the job run in bg or fg? */
    pid_t pid;       /* Process id */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return; /* Ignore empty lines */

    if (!builtin_command(argv))
    {
        if ((pid = fork()) == 0)
        { /* Child runs user job */
            if (bg != 0)
            {
                setpgid(getpid(), backpid);
            }
            else
            {
                setpgid(getpid(), forepid);
            }
            if ((findredirect(">", argv) == 1))
            {
                char *filo;
                filo = GetFileName(argv, ">");
                OutputRedirect(filo, ">");
            }
            if ((findredirect(">>", argv) == 1))
            {
                char *filo;
                filo = GetFileName(argv, ">>");
                OutputRedirect(filo, ">>");
            }
            if ((findredirect("<", argv) == 1))
            {
                char *fil;
                fil = GetFileName(argv, "<");
                InputRedirect(argv, fil);
            }
            findReplace(argv);
            if (execv(argv[0], argv) < 0)
            {
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
        else
        {    
            signal(SIGCHLD, _SignalHandler);
            sleep(1);
        }
    }
    
    return;
}
void twodParser(char **input, char *total[Max][Max])  //parses the string according to the "|"
{                                                   //into the 2d array
    int i = 0, p = 0, q = 0;
    char *a = "|";
    char *b = NULL;
    while (input[i] != b)
    {
        if (strcmp(input[i], a) == 0)
        {
            total[p][q] = NULL;
            p++;
            q = 0;
        }
        else
        {

            total[p][q] = input[i];
            q++;
        }

        i++;
    }
}
void pipefunc(char *total[Max][Max], int pipeCommands) //the function which implements multiple pipes
{
    
    char *b = NULL;
    int r = (pipeCommands - 1) * 2;   //number of file discriptors I will need
    int p = 0, i = 0;
    int pipeline[r];

    while (i < pipeCommands - 1)  //pipe2() is a fantastic thing which I discovered
    {                             //while reading the manpage of pipe(). it automatically closes fds'.
        if ((pipe2(pipeline + (2 * i), O_CLOEXEC) < 0))
            printf("pipe not setup");
        i++;
    }

    while (p < pipeCommands)  //run the loop and fork as long as you have commands
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            if (p == pipeCommands - 1)
            {
                if ((findredirect(">>", total[p]) == 1))
                {
                    char *filo;
                    filo = GetFileName(total[p], ">>");
                    OutputRedirect(filo, ">>");
                }
                if ((findredirect(">", total[p]) == 1))
                {
                    char *filo;
                    filo = GetFileName(total[p], ">");
                    OutputRedirect(filo, ">");
                }
                if ((findredirect("<", total[p]) == 1)) //if redirection of input is there than redirect the fd's
                {
                    char *fil;
                    fil = GetFileName(total[p], "<");
                    InputRedirect(total[p], fil);
                }
                findReplace(total[p]);
                dup2(pipeline[2 * (p - 1)], 0);
                execv(total[p][0], *(total + p));
            }
            else if (p > 0)
            {
                dup2(pipeline[2 * (p - 1)], 0);
                dup2(pipeline[(2 * p) + 1], 1);
                execv(total[p][0], *(total + p));
            }
            else
            {
                if ((findredirect("<", total[p]) == 1)) //if redirection of input is there than redirect the fd's
                {
                    char *fil;
                    fil = GetFileName(total[p], "<");
                    InputRedirect(total[p], fil);
                }
                findReplace(total[p]);
                dup2(pipeline[1], 1);
                execv(total[p][0], *(total + p));
            }
        }
        p++;
    }
    return;
}

int main()
{
    char input[Max];    //usr input
    char *token[Max];   // tokens of user input
    char input2[Max];   //copy of user input
    char *total[Max][Max];  // 2d array to store tokens according to the pipe
    int numPipe,bg=0;
    pid_t backpid, forepid;

    while (1)
    {
        signal(SIGINT, _SignalHandler);  // all signal handlers
        signal(SIGTERM, _SignalHandler);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCONT, _SignalHandler);

        write(1, "CS361 >", 7);

        fgets(input, Max, stdin);  //get input from the user
        strcpy(input2, input);     //make copy of the input
        bg=parseline(input, token);    //parse the line
        int numPipe = pipecheck(token);   //count the number of pipes
        setpgid(forepid, forepid);
        setpgid(backpid, backpid);
        if (bg != 0)    //if there is "&" then it is 1
        {
            setpgid(backpid, backpid);
        }
        else
        {
            setpgid(forepid, forepid);
        }
        tcsetpgrp(STDIN_FILENO, forepid);

        if (numPipe == 0)
        {
            eval(input2,backpid,forepid); 
        }
        else
        {                             //if there is pipe than fork it and do piping
            pid_t pid = fork();

            if (pid == 0)
            {
                if (bg != 0)
                {
                    setpgid(getpid(), backpid);
                }
                else
                {
                    setpgid(getpid(), forepid);
                }
                twodParser(token, total);   //parses the token into 2d array of inputs
                pipefunc(total, numPipe + 1);  //the function where pipe is implemented
                exit(0);  //send the signal that children have finished their work
            }
            else
            {
                signal(SIGCHLD, _SignalHandler);
                sleep(1); //wait for the child to print output and than continue
            }
        }
    }
    return 0;
}



       
