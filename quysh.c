/*
    This program is a custom shell named QuYsh.
    Its name originates from the "Quiche" which is a famous french dish and the "sh" from bash.
    As for the origin of the 'Y' linking the two words, it must remain a secret :x

    @ Authors:
        Corentin HUMBERT
        Paul LAMBERT
    
    @ Last Modification:
        31-10-2020 (DMY Formats)
 
    @ Version: 0.8
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
#define MAX_FORK 32

/* Shell command feedback constants */
#define OK_SIG 0
#define ERROR_SIG -1
#define EXIT_SIG 42

/* Shell GUI constants [EDITABLE BY USER] */
#define ENABLE_COLORS 1
#define HIDE_CWD 0
#define DEBUG 0

/* Pipes constants */
#define READ_END 0
#define WRITE_END 1

typedef struct path
{
    char *path_text;
    struct path *next;
} path_t, *pPath_t;

typedef struct paths
{
    int count;
    pPath_t first;
} paths_t, *pPaths_t;

/*
 * Structure: childProgram
 * -----------------------
 * Represents a child program running in the background
 * 
 *  id:   A serial number corresponding to its date of creation
 *        The bigger the ID, the younger the child is
 *  pid:  The PID of the child program
 *  argc: The number of arguments given to the child program
 *  argv: An array containing all the arguments given to the child program
 *  next: A pointer to the next child program
 *        NULL if the current child program is the youngest
 */
typedef struct childProgram
{
    int id;
    int pid;
    int argc;
    char **argv;
    struct childProgram *next;
} childProgram_t, *pChildProgram_t;

/*
 * Structure: programDescriptor
 * --------------------------
 * Holds information about all children program running in background
 * 
 *  children: The number of children currently running in background
 *  serialID: A global serial number
 *            The serialID is resetted to 0 whenever all children programs have ended
 *  first:    A pointer to the first child program
 *            NULL if there are currently no child program running in background
 */
typedef struct programDescriptor
{
    int children;
    int serialID;
    pChildProgram_t first;
} progDesc_t, *pProgDesc_t;

int parseCommand(char **cmd, pPaths_t paths, pProgDesc_t proDes);
int executeCommand(char *path, int argc, char **argv, char **envp, int state, pProgDesc_t proDes);
char *getPwd();
char *getBinPath(char *filename, pPaths_t paths);
int fileExists(char *filename);
int printShellPrefix();

pProgDesc_t newProgramDescriptor();
void freeProgramDescriptor(pProgDesc_t proDes);
pChildProgram_t addProgram(int pid, int argc, char **argv, pProgDesc_t proDes);
int removeProgram(int id, pProgDesc_t proDes);
pChildProgram_t findProgram(int pid, pProgDesc_t proDes);

int main(int argc, char **argv, char **envp)
{
    if (DEBUG)
        printf("--< DEBUG mode is activated >--\n\n");

    const char *PATH_RAW = getenv("PATH");
    const int PATH_RAW_LEN = strlen(PATH_RAW);
    const char PATH_DELIM[2] = ":";

    char *PATH_RAW_CPY = (char *)malloc(PATH_RAW_LEN * sizeof(char));

    char *str_it;    // An iterator over the RAW_PATH where elements are delimited by the PATH_DELIM character
    pPath_t path_it; // A path iterator

    pPaths_t paths = (pPaths_t)malloc(sizeof(paths_t));
    paths->count = 0;
    paths->first = NULL;

    // Computes a copy of the RAW_PATH
    strcpy(PATH_RAW_CPY, PATH_RAW);

    if (DEBUG)
        printf("All paths: \n");

    // Splits the RAW_PATH into multiple individual path structures
    str_it = strtok(PATH_RAW_CPY, PATH_DELIM);
    path_it = (pPath_t)malloc(sizeof(path_t));
    while (str_it != NULL)
    {
        // Initializes a path
        path_it->path_text = (char *)malloc(MAX_PATH_LEN * sizeof(char));
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
            path_it->next = (pPath_t)malloc(sizeof(path_t));
        path_it = path_it->next;
    }

    pProgDesc_t proDes = newProgramDescriptor();

    // Shell Loop
    for (;;)
    {
        // Looks for terminated children
        int status;
        int childPid;

        while ((childPid = waitpid(-1, &status, WNOHANG)) > 0)
        {
            pChildProgram_t child = findProgram(childPid, proDes);

            if (child != NULL)
            {
                printf("[%d]  Done\t\t", child->id);
                for (int i = 0; i < child->argc; i++)
                {
                    printf("%s ", child->argv[i]);
                }
                printf("\n");

                removeProgram(child->id, proDes);
            }
            else
            {
                printf("Couldn't find %d", childPid);
                exit(-1);
            }
        }

        if (DEBUG)
        {
            if (childPid != -1)
            {
                if (childPid == 0)
                {
                    printf("My heirs are doing me proud.\n");
                }
                else
                {
                    printf("Ended %d... They will be remembered. [%d]\n", childPid, status);
                }
            }
        }

        printShellPrefix();
        fflush(stdout);
        char *line = readline();

        int fb = parseCommand(split_in_words(line), paths, proDes);

        if (fb == EXIT_SIG)
            break;

        free(line);
    }

    freeProgramDescriptor(proDes);

    // Frees the allocated memory
    path_it = paths->first;
    pPath_t prev_pt;
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
 * Function: parseCommand
 * ----------------------
 * Evaluates a given command
 *
 *  line:    The user input
 *  paths:   The structure containing all paths referenced in the PATH environement variable
 *
 *  Returns: 0 if the command was processed correctly
 *           EXIT_SIG if the user wants to exit the Shell 
 */
int parseCommand(char **cmd, pPaths_t paths, pProgDesc_t proDes)
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

                        executeCommand(binPath, subArgC, cmd, __environ, binState, proDes);
                        if (subArgC + 1 < argCount)
                            parseCommand(&(cmd[subArgC + 1]), paths, proDes);
                        break;
                    }
                }

                if (binState == BIN_FG)
                    executeCommand(binPath, subArgC, cmd, __environ, binState, proDes);
            }

            free(binPath);
        }
    }

    return fb;
}

// TODO: Try and find a way to free the child argvCpy
/*
 * Function: executeCommand
 * ------------------------
 * Launches a specific binary in a specific state
 *
 *  path:    The complete path of the binary to be launched
 *  argc:    The number of arguments
 *  argv:    An array containing all the arguments
 *  envp:    An array containing the environment variables
 *  state:   The state in which the binary has to be launched (BIN_FG to launch normally or BIN_BG to launch it in the background)
 *
 *  Returns: 0 if all went well
 */
int executeCommand(char *path, int argc, char **argv, char **envp, int state, pProgDesc_t proDes)
{
    int status, waitRes;
    char **argvCpy;
    int childPid = fork(); // Creates a child process

    // Identifies who is the current process and performs specific tasks accordingly
    switch (childPid)
    {
    case -1: // An error occured
        perror("fork: ");
        exit(-1);
    case 0: // The current process is a child
        // Makes a copy of argv
        argvCpy = (char **)(malloc((argc + 1) * sizeof(char *)));
        for (int i = 0; i < argc; i++)
        {
            argvCpy[i] = (char *)malloc(sizeof(char *));
            strcpy(argvCpy[i], argv[i]);
        }
        argvCpy[argc] = NULL; // Very important!!

        execve(path, argvCpy, envp);
        perror("execve failed");
        exit(EXIT_FAILURE);
        break;
    default: // The current process is the parent
        if (DEBUG)
            printf("Is that you [%s] %d? Your father's right here kiddo!\n", path, childPid);

        // If the child process has been executed normally, waits for it to terminate
        if (state == BIN_FG)
        {
            waitRes = waitpid(childPid, &status, 0);
            if (waitRes != -1)
            {
                if (DEBUG)
                    printf("My child %d has served his country well. [%d]\n", waitRes, status);
            }
            else
            {
                perror("waitpid: ");
            }
        }
        else if (state == BIN_BG)
        {

            pChildProgram_t child = addProgram(childPid, argc, argv, proDes);
            printf("[%d] %d\n", child->id, child->pid);
        }

        break;
    }

    return 0;
}

/*
 * Function: getPwd
 * ----------------
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
 *  paths:    The structure containing all paths referenced in the PATH environement variable
 *
 *  Returns:  The complete path of a binary if it exists in one of the directories of paths
 *            NULL if the binary could not be found
 */
char *getBinPath(char *filename, pPaths_t paths)
{
    char *binaryPath = (char *)malloc(MAX_PATH_LEN * sizeof(char));
    pPath_t path_it = paths->first;

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
 *  Returns:  0 if the file does not exist
 *            1 if the file exists
 */
inline int fileExists(char *filename)
{
    return (access(filename, F_OK) != -1);
}

/*
 * Function: printShellPrefix
 * --------------------------
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

/*
 * Function: newProgramDescriptor
 * ------------------------------
 * Creates a new Program Descriptor
 *
 *  Returns: A pointer to the newly allocated Program Descriptor
 */
pProgDesc_t newProgramDescriptor()
{
    pProgDesc_t proDes = (pProgDesc_t)malloc(sizeof(progDesc_t));
    proDes->children = 0;
    proDes->serialID = 0;
    proDes->first = NULL;
    return proDes;
}

/*
 * Function: freeProgramDescriptor
 * -------------------------------
 * Deallocates a Program Descriptor
 *
 *  proDes: A pointer to the Program Descriptor
 */
void freeProgramDescriptor(pProgDesc_t proDes)
{
    pChildProgram_t it = proDes->first;
    pChildProgram_t prev = NULL;

    while (it != NULL)
    {
        for (int i = 0; i < it->argc; i++)
        {
            free(it->argv[i]);
        }
        free(it->argv);
        prev = it;
        it = it->next;
        free(prev);
    }
    free(proDes);
}

/*
 * Function: addProgram
 * --------------------
 * Adds a specific child program to the list of children running in background
 * 
 *  pid:     The PID of the child program
 *  argc:    The number of arguments given to the child program
 *  argv:    An array containing all the arguments given to the child program
 *  proDes:  A pointer to the Program Descriptor
 *
 *  Returns: A pointer to the newly allocated Child Program
 */
pChildProgram_t addProgram(int pid, int argc, char **argv, pProgDesc_t proDes)
{
    pChildProgram_t it = proDes->first;
    pChildProgram_t prev = NULL;

    while (it != NULL)
    {
        prev = it;
        it = it->next;
    }

    it = (pChildProgram_t)malloc(sizeof(childProgram_t));
    it->id = ++proDes->serialID;
    it->pid = pid;
    it->next = NULL;

    it->argc = argc;
    it->argv = malloc(sizeof(char **));

    for (int i = 0; i < argc; i++)
    {
        it->argv[i] = malloc(strlen(argv[i]) * sizeof(char));
        strcpy(it->argv[i], argv[i]);
    }

    if (prev != NULL)
        prev->next = it;
    else
        proDes->first = it;

    proDes->children++;

    return it;
}

/*
 * Function: removeProgram
 * -----------------------
 * Removes a specific child program from the list of children running in background
 * 
 *  id:      The ID of the child program
 *  proDes:  A pointer to the Program Descriptor
 *MAX_FORK
 *  Returns: -1 if an error occured
 *           0 if the child could not be removed
 *           1 if the child has been successfully removed
 */
int removeProgram(int id, pProgDesc_t proDes)
{
    pChildProgram_t it = proDes->first;
    pChildProgram_t prev = NULL;

    if (proDes->children == 0)
    {
        printf("Can't remove program from descriptor since there are no program left\n");
        return -1;
    }

    while (it != NULL)
    {
        if (it->id == id)
        {
            if (prev != NULL)
                prev->next = it->next;
            else
                proDes->first = it->next;

            for (int i = 0; i < it->argc; i++)
            {
                free(it->argv[i]);
            }
            free(it->argv);
            free(it);

            proDes->children--;

            if (proDes->children == 0)
                proDes->serialID = 0;

            return 1;
        }
        prev = it;
        it = it->next;
    }

    return 0;
}

/*
 * Function: findProgram
 * ---------------------
 * Searches for a specific child program in the list of children running in background
 * 
 *  pid:     The PID of the child program to find
 *  proDes:  A pointer to the Program Descriptor
 *
 *  Returns: A pointer to the child program matching the PID given in argument
 *           NULL if the child program could not be found
 */
pChildProgram_t findProgram(int pid, pProgDesc_t proDes)
{
    pChildProgram_t it = proDes->first;

    while (it != NULL)
    {
        if (it->pid == pid)
            return it;

        it = it->next;
    }

    return NULL;
}