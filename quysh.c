#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readline.h"

#define SHELL_NAME "quysh"
#define DEBUG 0

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
int execProcess(char *path, char **argv, char **envp);
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
        path_it->path_text = malloc(sizeof(char));
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

    if (DEBUG)
    {
        path_it = paths->first;
        while (path_it != NULL)
        {
            int i = 0;
            printf("\t%s :\n\n\t\t", path_it->path_text);
            while (path_it->path_text[i] != 0)
            {
                printf("%d(%c) ", path_it->path_text[i], path_it->path_text[i]);
                i++;
            }
            printf("\n\n");
            path_it = path_it->next;
        }
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

        processCommand(line, paths, envp);

        free(line);
    }
    return 0;
}

int processCommand(char *line, ppaths_t paths, char **envp)
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
    else if (strcmp(words[0], "print") == 0)
    {
        //int i = 0;
        printf("%s\n", envp[0]);
        /*
        while (envp[i] != NULL) {
            char* env_var = envp[i];
            printf("%s\n", env_var);
        }*/
    }
    else
    {
        char *binPath = getBinPath(words[0], paths);

        if (binPath != NULL)
            execProcess(binPath, words, envp);
        else
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

char *getPwd()
{
    char cwd[1024];
    return getcwd(cwd, sizeof(cwd));
}

// TODO: Last path is broken
char *getBinPath(char *filename, ppaths_t paths)
{
    char *binaryPath = malloc(1024 * sizeof(char));
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

int fileExists(char *filename)
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
