#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readline.h"

#define SHELL_NAME "quysh"
#define DEBUG 0

int processCommand(char *line, char argc, char **argv, char **envp);
int execProcess(char *path, char **argv, char **envp);
char *getPwd();
int printShellPrefix();

int main(int argc, char **argv, char **envp)
{
    /*for (int i=0;envp[i]!=NULL;i++)
    printf("env[%d]=%s\n",i,envp[i]);
    printf("\n");*/

    // set stdout without buffering so what is printed
    // is printed immediately on the screen.
    // setvbuf(stdout, NULL, _IONBF, 0);
    // setbuf(stdout, NULL);

    for (;;)
    {
        printShellPrefix();
        fflush(stdout);
        char *line = readline();

        processCommand(line, argc, argv, envp);

        free(line);
    }
    return 0;
}

int processCommand(char *line, char argc, char **argv, char **envp)
{
    char **words = split_in_words(line);

    if (DEBUG)
    {
        printf("%s\n", line);
        for (int i = 0; words[i] != NULL; i++)
            printf("[%s], ", words[i]);
        printf("\n");
    }

    if (strcmp(words[0], "cd") == 0)
    {
        if (words[2] != NULL)
        {
            printf("quysh: cd: too many arguments\n");
        }
        else
        {
            if (chdir(words[1]) == -1)
            {
                printf("quysh: cd: No such file or directory\n");
            }
        }
    }
    else if (strcmp(words[0], "pwd") == 0)
    {
        printf("%s\n", getPwd());
    }
    else if (strcmp(words[0], "ls") == 0)
    {
        char *args[3];
        /*
        args[0] = "ls";
        args[1] = "-al";
        args[2] = argv[1];*/
        execProcess("/bin/ls", words, envp);
    }
    else if (strcmp(words[0], "print") == 0) {
        int i = 0;
        printf("%s\n", envp[0]);
        /*
        while (envp[i] != NULL) {
            char* env_var = envp[i];
            printf("%s\n", env_var);
        }*/
    }
    else
    {
        printf("\nCommand '%s' not found.\n\nTry: sudo apt install <deb name>\n\n", words[0]);
    }

    free(words);

    return 0;
}

int execProcess(char *path, char **argv, char **envp)
{
    int status;
    int c_pid = fork();
    int wait_res;
    switch (c_pid)
    {
    case -1:
        printf("FATAL!\n");
        break;
    case 0:
        if (DEBUG)
            printf("Who is my father?\n");
        execve(path, argv, envp);
        break;
    default:
        if (DEBUG)
            printf("Is that you %d? Your father's right here kiddo!\n", c_pid);
        wait_res = wait(&status);
        if (wait_res > 0)
        {
            if (DEBUG)
                printf("My child %d has served his country well\n", wait_res);
        }
        break;
    }
    return 0;
}

int isCommand(char** cmd_obj, char* name, int min_args, int max_args) {
    if (strcmp(cmd_obj[0], name) != 0)
        return 0;
    
    if (cmd_obj[min_args] == NULL)
        return 0;

    if (cmd_obj[max_args + 1] != NULL)
        return 0;

    return 1;
}

char *getPwd()
{
    char cwd[1024];
    return getcwd(cwd, sizeof(cwd));
}

int file_exists (char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}

int printShellPrefix()
{
    char *user = "root";
    char *cwd = getPwd();

    printf("\033[1;33m");
    printf("%s@%s", user, SHELL_NAME);
    printf("\033[0m");

    printf(":");
    
    printf("\033[1;36m");
    printf("%s", cwd);
    printf("\033[0m");

    printf(" > ");

    return 0;
}
