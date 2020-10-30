/*
    This program is a custom shell named QuYsh.
    Its name originates from the "Quiche" which is a famous french dish and the "sh" from bash.
    As for the origin of the 'Y' linking the two words, it must remain a secret :x

    @ Authors:
        Corentin HUMBERT
        Paul LAMBERT
    
    @ Last Modification:
        30-10-2020 (DMY Formats)
 
    @ Version: 0.77c (Sexy Version)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readline.h"

#define SHELL_NAME "quysh"

#define BIN_FG 0
#define BIN_BG 1

#define MAX_PATH_LEN 4096

#define OK_SIG 0
#define ERROR_SIG -1
#define EXIT_SIG 42

#define ENABLE_COLORS 1
#define HIDE_CWD 0
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

int processCommand(char **cmd, ppaths_t paths);
int execBinary(char *path, int argc, char **argv, char **envp, int state);
char *getPwd();
char *getBinPath(char *filename, ppaths_t paths);
int fileExists(char *filename);
int printShellPrefix();

int main(int argc, char **argv, char **envp)
{
    if (DEBUG)
        printf("--< DEBUG mode is activated >--\n\n");

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

    // Shell Loop
    for (;;)
    {
        // Looks for terminated children
        int status;
        int wait_res = waitpid(-1, &status, WNOHANG);

        if (DEBUG)
        {
            if (wait_res != -1)
            {
                if (wait_res == 0)
                {
                    printf("My heirs are doing me proud.\n");
                }
                else
                {
                    printf("Ended %d... They will be remembered. [%d]\n", wait_res, status);
                }
            }
        }

        printShellPrefix();
        fflush(stdout);
        char *line = readline();

        int fb = processCommand(split_in_words(line), paths);

        if (fb == EXIT_SIG)
            break;

        free(line);
    }

    // Frees the allocated memory
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

    /*
    for (int i = 0; envp[i] != NULL; i++)
        printf("env[%d]=%s\n", i, envp[i]);
    printf("\n");*/

    // set stdout without buffering so what is printed
    // is printed immediately on the screen.
    //setvbuf(stdout, NULL, _IONBF, 0);
    //setbuf(stdout, NULL);

    return 0;
}

/*
 * Function: processCommand
 * --------------------
 * Evaluates a given command
 *
 *  line: The user input
 *  paths: The structure containing all paths referenced in the PATH environement variable
 *
 *  Returns: 0 if the command was processed correctly
 *           EXIT_SIG if the user wants to exit the Shell 
 */
int processCommand(char **cmd, ppaths_t paths)
{
    int fb = OK_SIG;
    int argCount;

    // Counts command arguments (command name included)
    for (argCount = 0; cmd[argCount] != NULL; argCount++)
        ;

    if (DEBUG)
    {
        printf("Command: %s [%d arg(s) provided]\n", cmd[0], argCount);
        for (int i = 0; i < argCount; i++)
            printf("\targ[%d]='%s'\n", i, cmd[i]);
        printf("\n");
    }

    if (argCount >= 1)
    {
        if (cmd[0][0] == '&')
        {
            printf("%s: syntax error near unexpected token `%s'\n", SHELL_NAME, cmd[0]);
            fb = ERROR_SIG;
        }
        else if (strcmp(cmd[0], "cd") == 0)
        {
            if (argCount <= 2)
            {
                if (chdir(cmd[1]) == -1)
                {
                    printf("%s: cd: No such file or directory\n", SHELL_NAME);
                }
            }
            else
            {
                printf("%s: cd: too many arguments\n", SHELL_NAME);
                fb = ERROR_SIG;
            }
        }
        else if (strcmp(cmd[0], "print") == 0)
        {
            if (argCount <= 2)
            {
                if (argCount == 1)
                {
                    for (int i = 0; __environ[i] != NULL; i++)
                        printf("%s\n", __environ[i]);
                }
                else
                {
                    char *var = getenv(cmd[1]);

                    if (var == NULL)
                        printf("\n");
                    else
                        printf("%s\n", getenv(cmd[1]));
                }
            }
            else
            {
                printf("%s: print: too many arguments\n", SHELL_NAME);
                fb = -1;
            }
        }
        else if (strcmp(cmd[0], "set") == 0)
        {
            if (argCount == 3)
            {
                setenv(cmd[1], cmd[2], 1);
            }
            else if (argCount < 3)
            {
                printf("%s: set: not enough arguments\n", SHELL_NAME);
                fb = ERROR_SIG;
            }
            else
            {
                printf("%s: set: too many arguments\n", SHELL_NAME);
                fb = ERROR_SIG;
            }
        }
        else if (strcmp(cmd[0], "exit") == 0)
        {
            fb = EXIT_SIG;
        }
        else
        {
            char *binPath = getBinPath(cmd[0], paths);
            int binState = BIN_FG;
            int subArgC = 0;

            if (binPath == NULL)
            {
                printf("%s: command not found\n", cmd[0]);
            }
            else
            {
                for (subArgC = 0; subArgC < argCount; subArgC++)
                {
                    if (strcmp(cmd[subArgC], "&") == 0)
                    {
                        binState = BIN_BG;

                        execBinary(binPath, subArgC, cmd, __environ, binState);
                        if (subArgC + 1 < argCount)
                            processCommand(&(cmd[subArgC + 1]), paths);
                        break;
                    }
                }

                if (binState == BIN_FG)
                    execBinary(binPath, subArgC, cmd, __environ, binState);
            }

            free(binPath);
        }
    }

    return fb;
}

// TODO: Try and find a way to free the child argvCpy
/*
 * Function: execBinary
 * --------------------
 * Launches a specific binary in a specific state
 *
 *  path: The complete path of the binary to be launched
 *  argc: The number of arguments
 *  argv: An array containing all the arguments
 *  envp: An array containing the environment variables
 *  state: The state in which the binary has to be launched (BIN_FG to launch normally or BIN_BG to launch it in the background)
 *
 *  Returns: 0 if all went well
 */
int execBinary(char *path, int argc, char **argv, char **envp, int state)
{
    int status, wait_res;
    char **argvCpy;
    int c_pid = fork(); // Creates a child process

    // Identifies who is the current process and performs specific tasks accordingly
    switch (c_pid)
    {
    case -1: // An error occured
        perror("fork: ");
        exit(-1);
    case 0: // The current process is a child
        // Makes a copy of argv
        argvCpy = malloc(argc * sizeof(char **));
        for (int i = 0; i < argc; i++)
        {
            argvCpy[i] = malloc(sizeof(char *));
            strcpy(argvCpy[i], argv[i]);
        }

        execve(path, argvCpy, envp);
        break;
    default: // The current process is the parent
        if (DEBUG)
            printf("Is that you [%s] %d? Your father's right here kiddo!\n", path, c_pid);

        // If the child process has been executed normally, waits for it to terminate
        if (state == BIN_FG)
        {
            wait_res = waitpid(c_pid, &status, 0);
            if (wait_res != -1)
            {
                if (DEBUG)
                    printf("My child %d has served his country well. [%d]\n", wait_res, status);
            }
            else
            {
                perror("waitpid: ");
            }
        }

        break;
    }

    return 0;
}

/*
 * Function: getPwd
 * --------------------
 * Returns a char* corresponding the current working directory
 *
 *  Returns: The current working directory
 */
inline char *getPwd()
{
    char cwd[MAX_PATH_LEN];
    return getcwd(cwd, sizeof(cwd));
}

/*
 * Function: getBinPath
 * --------------------
 * Looks for the complete file path of a specific binary by going through the list of paths
 *
 *  filename: The name of the binary to find
 *  paths: The structure containing all paths referenced in the PATH environement variable
 *
 *  Returns: The complete path of a binary if it exists in one of the directories of paths
 *           NULL if the binary could not be found
 */
char *getBinPath(char *filename, ppaths_t paths)
{
    char *binaryPath = malloc(MAX_PATH_LEN * sizeof(char));
    ppath_t path_it = paths->first;

    // Iterates over the Linked List of paths
    while (path_it != NULL)
    {
        // Builds a possible complete path
        strcpy(binaryPath, path_it->path_text);
        strcat(binaryPath, "/");
        strcat(binaryPath, filename);

        // If the binary exists, returns its complete path
        if (fileExists(binaryPath))
        {
            if (DEBUG)
                printf("Located '%s' at \"%s\"\n", filename, binaryPath);

            return binaryPath;
        }
        path_it = path_it->next;
    }
    return NULL; // The binary has not been found
}

/*
 * Function: fileExists
 * --------------------
 * Checks whether or not a given file exists in the current working directory
 *
 *  filename: The name of the file to check
 *
 *  Returns: 0 if the file does not exist
 *           1 if the file exists
 */
inline int fileExists(char *filename)
{
    return (access(filename, F_OK) != -1);
}

/*
 * Function: printShellPrefix
 * --------------------
 * Solely used for graphics. Echoes basic information to try and match the look of the prompt of the original Shell
 *
 *  Returns: 0 if everything went well
 */
int printShellPrefix()
{
    char username[32];
    char *cwd = getPwd(); // The current working directory

    strcpy(username, getenv("USER"));

    int cwdLen = strlen(cwd);
    int usernameLen = strlen(username);

    // Truncates "/home/user" from displayed cwd in prefix
    for (int i = 0; i < cwdLen; i++)
    {
        if (cwd[i] == '/')
        {
            if (strncmp(&(cwd[i + 1]), username, usernameLen) == 0)
            {
                cwd = &(cwd[i + 1 + usernameLen]);
                break;
            }
        }
    }

    if (ENABLE_COLORS)
    {
        printf("\033[1;33m");
        printf("%s@%s", username, SHELL_NAME);
        printf("\033[0m");

        if (!HIDE_CWD)
        {
            printf(":");

            printf("\033[1;36m");
            printf("~%s", cwd);
            printf("\033[0m");
        }
    }
    else
    {
        printf("%s@%s", username, SHELL_NAME);

        if (!HIDE_CWD)
        {
            printf(":~%s", cwd);
        }
    }

    printf(" > ");

    return 0;
}
