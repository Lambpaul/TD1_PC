/*
    This program is a custom shell named QuYsh.
    Its name originates from the "Quiche" which is a famous french dish and the "sh" from bash.
    As for the origin of the 'Y' linking the two words, it must remains a secret :x

    @ Authors:
        Corentin HUMBERT
        Paul LAMBERT
    
    @ Last Modification:
        24-07-2020 (DMY Formats)
 
    @ Version: 0.4
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readline.h"

#define MAX_PATH_LEN 4096
#define EXIT_SIG 42
#define SHELL_NAME "quysh"
#define DEBUG 0

#define BIN_FOREGROUND 0
#define BIN_BACKGROUND 1

typedef struct path
{
    char *path_text;
    struct path *next;
} path_t, *ppath_t;

typedef struct paths
{
    int count;
    ppath_t first;
} paths_t, *ppaths_t;

int processCommand(char *line, ppaths_t paths, char **envp);
int execProcess(char *path, int state, char **argv, char **envp);
char *getPwd();
int printShellPrefix();
int fileExists(char *filename);
char *getBinPath(char *filename, ppaths_t paths);

int main(int argc, char **argv, char **envp)
{
    const char *PATH_RAW = getenv("PATH");     // ??
    const int PATH_RAW_LEN = strlen(PATH_RAW); // O(n)
    const char PATH_DELIM[2] = ":";

    char *PATH_RAW_CPY = malloc(PATH_RAW_LEN * sizeof(char));

    char *str_it;    // An iterator over the RAW_PATH where elements are delimited by the PATH_DELIM character
    ppath_t path_it; // A path iterator

    ppaths_t paths = malloc(sizeof(paths_t));
    paths->count = 0;
    paths->first = NULL;

    // Computes a copy of the RAW_PATH
    strcpy(PATH_RAW_CPY, PATH_RAW);

    if (DEBUG)
        printf("All paths: \n");

    // Splits the RAW_PATH into multiple individual path structures
    str_it = strtok(PATH_RAW_CPY, PATH_DELIM);
    path_it = malloc(sizeof(path_t));
    while (str_it != NULL)
    {
        // Initializes a path
        path_it->path_text = malloc(MAX_PATH_LEN * sizeof(char));
        path_it->next = NULL;

        // References the first path to the paths structure
        if (paths->count == 0)
            paths->first = path_it;

        // Copies the values from the RAW_PATH iterator to our custom path structure
        strcpy(path_it->path_text, str_it);
        paths->count++;

        if (DEBUG)
            printf("\t%s\n", path_it->path_text);

        // Continues to the next entry
        str_it = strtok(NULL, PATH_DELIM);
        if (str_it != NULL)
            path_it->next = malloc(sizeof(path_t));
        path_it = path_it->next;
    }

    /*
    for (int i = 0; envp[i] != NULL; i++)
        printf("env[%d]=%s\n", i, envp[i]);
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

        int fb = processCommand(line, paths, envp);
        if (fb == EXIT_SIG)
            break;

        free(line);
    }

    path_it = paths->first;
    ppath_t prev_pt;

    while (path_it != NULL)
    {
        free(path_it->path_text);
        prev_pt = path_it;
        path_it = path_it->next;
        free(prev_pt);
    }
    free(paths);

    free(PATH_RAW_CPY);

    return 0;
}

int processCommand(char *line, ppaths_t paths, char **envp)
{
    char **words = split_in_words(line);
    int fb = 0;

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
    else if (strcmp(words[0], "print") == 0)
    {
        if (words[2] == NULL)
        {
            if (words[1] != NULL)
            {
                if (strcmp(words[1], "PATH") == 0)
                {
                    ppath_t path_it = paths->first;
                    while (path_it != NULL)
                    {
                        printf("%s\n", path_it->path_text);
                        path_it = path_it->next;
                    }
                }
                else
                {
                    printf("Print: Invalid argument. Available argument: [], [PATH]\n");
                }
            }
            else
            {
                for (int i = 0; envp[i] != NULL; i++)
                    printf("env[%d]=%s\n", i, envp[i]);
            }
        }
        else
        {
            printf("Print: Invalid argument. Available argument: [], [PATH]\n");
        }

        printf("\n");
    }
    else if (strcmp(words[0], "set") == 0)
    {
        if (words[3] == NULL)
        {
            if (strcmp(words[1], "PATH") == 0)
            {
                printf("%s\n", words[2]);
            }
        }
    }
    else if (strcmp(words[0], "exit") == 0)
    {
        fb = EXIT_SIG;
    }
    else
    {
        char *binPath = getBinPath(words[0], paths);

        if (binPath != NULL)
        {
            int binState = BIN_FOREGROUND;

            for (int i = 0; words[i] != NULL; i++)
            {
                if (strcmp(words[i], "&") == 0)
                    binState = BIN_BACKGROUND;
            }
            printf("BIN STATE : %i\n", binState);
            execProcess(binPath, binState, words, envp);
        }
        else
            printf("\nCommand '%s' not found.\n\nTry: sudo apt install <deb name>\n\n", words[0]);

        free(binPath);
    }

    free(words);

    return fb;
}

// TODO: & -> background is broken
int execProcess(char *path, int state, char **argv, char **envp)
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

        if (state == BIN_FOREGROUND)
        {
            wait_res = wait(&status);
            if (wait_res > 0)
            {
                if (DEBUG)
                    printf("My child %d has served his country well\n", wait_res);
            }
        }

        break;
    }
    return 0;
}

int isCommand(char **cmd_obj, char *name, int min_args, int max_args)
{
    if (strcmp(cmd_obj[0], name) != 0)
        return 0;

    if (cmd_obj[min_args] == NULL)
        return 0;

    if (cmd_obj[max_args + 1] != NULL)
        return 0;

    return 1;
}

inline char *getPwd()
{
    char cwd[MAX_PATH_LEN];
    return getcwd(cwd, sizeof(cwd));
}

// TODO: Last path is broken
char *getBinPath(char *filename, ppaths_t paths)
{
    char *binaryPath = malloc(MAX_PATH_LEN * sizeof(char));
    ppath_t path_it = paths->first;

    while (path_it != NULL)
    {
        strcpy(binaryPath, path_it->path_text);
        strcat(binaryPath, "/");
        strcat(binaryPath, filename);

        if (fileExists(binaryPath))
        {
            if (DEBUG)
                printf("Located %s at %s\n", filename, binaryPath);

            return binaryPath;
        }
        path_it = path_it->next;
    }
    return NULL;
}

inline int fileExists(char *filename)
{
    return (access(filename, F_OK) != -1);
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
