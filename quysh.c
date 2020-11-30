/*
    This program is a custom shell named QuYsh.
    Its name originates from the "Quiche" which is a famous french dish and the "sh" from bash.
    As for the 'Y' linking the two words, its origin is kept inside of the Pandora's box.

    @ Authors:
        Corentin HUMBERT
        Paul LAMBERT
    
    @ Last Modification:
        30-11-2020 (DMY Formats)
 
    @ Version: 1.0.0 (Beyond-Grub)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readline.h"

#define SHELL_NAME "quysh"

/* Process state */
#define BIN_FG 0 // Runs in foreground
#define BIN_BG 1 // Runs in background

/* Process in/out mode */
#define PIP_NONE 0  // Current process does not use any pipe
#define PIP_WRITE 1 // Current process uses a pipe to pass its STDOUT to the next process
#define PIP_READ 2  // Current process used a pipe to get its STDIN from the previous process
#define PIP_BOTH 3  // Current process is both reading and writing from external processes

/* Process file output mode */
#define RED_NONE 0 // Current process does not output to a file
#define RED_OVER 1 // Current process outputs to a file by overriding it
#define RED_APPE 2 // Current process outputs to a file by appending to it

/* Pipe ends */
#define READ_END 0
#define WRITE_END 1

int pipeA[2] = {-1, -1};
int pipeB[2] = {-1, -1};

/* Shell basic constants */
#define MAX_PATH_LEN 4096
#define MAX_FORK 32 // TODO: UNUSED

/* Shell command feedback constants */
#define ERROR_SIG -1
#define OK_SIG 0
#define EXIT_SIG 42

const int SHELL_OPE_COUNT = 3;
const char SHELL_OPE[3] = {'&', '|', '>'};

/* Shell GUI constants [EDITABLE BY USER] */
#define ENABLE_COLORS 1
#define HIDE_CWD 0
#define DEBUG 0

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

int parseCommand(char **cmd, pPaths_t paths, int readFromPipe, int pipeCount, pProgDesc_t proDes);
int executeCommand(char *path, int argc, char **argv, char **envp, int state, int pipeState, int getPipe[2], int givePipe[2], int redirState, char *outFile, pProgDesc_t proDes);
char *getPwd();
char *getBinPath(char *filename, pPaths_t paths);
int fileExists(char *filename);
int isOperator(char c);
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

        int fb = parseCommand(split_in_words(line), paths, 0, 0, proDes); // There is no pipe at the start

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

    return 0;
}

/*
 * Function: parseCommand
 * ----------------------
 * Evaluates a given command
 *
 *  line:           The user input
 *  paths:          The structure containing all paths referenced in the PATH environement variable
 *  readFromPipe:   1 if the given command was prefixed by a '|'
 *                  0 otherwise
 *  pipeCount:      The number of pipes preceeding the given command
 *
 *  Returns: 0 if the command was processed correctly
 *           EXIT_SIG if the user wants to exit the Shell 
 */
int parseCommand(char **cmd, pPaths_t paths, int readFromPipe, int pipeCount, pProgDesc_t proDes)
{
    int fb = OK_SIG;
    int argCount;
    int localArgsCount = 0;
    int reached = 0;

    // Counts command arguments (command name included)
    // And replaces the "~" character in commands by "/home/usr" as well
    for (argCount = 0; cmd[argCount] != NULL; argCount++)
    {
        if (strcmp(cmd[argCount], "~") == 0)
            cmd[argCount] = getenv("HOME");

        /* If '$X' is found, we treat the associated string X as a potential
         * environment variable and replace '$X' with the value of 'X'
         */
        if (cmd[argCount][0] == '$')
        {
            char *pt = cmd[argCount];
            const char *envVarName = pt + 1;
            char *envVar = getenv(envVarName);

            if (envVar == NULL)
                envVar = "";

            cmd[argCount] = envVar;
        }

        if (!reached && (strcmp(cmd[argCount], "&") == 0 || strcmp(cmd[argCount], "|") == 0))
            reached = 1;

        if (!reached)
            localArgsCount++;
    }

    if (DEBUG)
    {
        printf("Command: %s [%d arg(s) provided]\n", cmd[0], argCount);
        for (int i = 0; i < argCount; i++)
            printf("\targ[%d]='%s'\n", i, cmd[i]);
        printf("\n");
    }

    if (argCount >= 1)
    {
        if (isOperator(cmd[0][0]))
        {
            printf("%s: syntax error near unexpected token `%s'\n", SHELL_NAME, cmd[0]);
            fb = ERROR_SIG;
        }
        else if (strcmp(cmd[0], "cd") == 0) // TODO: For cd and set, argCount will lead to command failure if there are other commands on the same line
        {
            if (localArgsCount <= 2)
            {
                if (localArgsCount == 1)
                    cmd[1] = getenv("HOME");

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
        else if (strcmp(cmd[0], "set") == 0)
        {
            if (localArgsCount == 3)
            {
                setenv(cmd[1], cmd[2], 1);
            }
            else if (localArgsCount < 3)
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
            if (DEBUG)
                printf("%s: Successfully exited\n", SHELL_NAME);
            fb = EXIT_SIG;
        }
        else
        {
            char *binPath = getBinPath(cmd[0], paths);
            int binState = BIN_FG;
            int subArgC = 0;
            int pipeState;
            int piping = 0;
            int *getPipe, *givePipe;

            // Swaps the pipes as explained further below
            if (pipeCount % 2 == 0)
            {
                getPipe = pipeB;
                givePipe = pipeA;
            }
            else
            {
                getPipe = pipeA;
                givePipe = pipeB;
            }

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

                        if (readFromPipe) // Prefixed by '|'
                            pipeState = PIP_READ;
                        else
                            pipeState = PIP_NONE;

                        executeCommand(binPath, subArgC, cmd, __environ, binState, pipeState, getPipe, givePipe, RED_NONE, NULL, proDes);

                        if (subArgC + 1 < argCount)
                            parseCommand(&(cmd[subArgC + 1]), paths, 0, pipeCount, proDes); // Since we read a "&", this command won't cannot pipe to the next one
                        break;
                    }
                    else if (strcmp(cmd[subArgC], "|") == 0) // The command will AT LEAST write to STDOUT
                    {
                        // The current command is going to send its output to the next one
                        piping = 1;

                        // TODO: Test if last command is of the form: cmd |
                        // This could prove troublesome since no command is placed after
                        // Try in Linux Shell to get ideas
                        // NB: I tried it and it does not seem to break anything but make sure to test things anyway

                        /* 
                         * In order to allow for a command to receive its input from its predecessor and send its output to its successor, it is required
                         * to use another pipe. So, each command will use two pipes: one for its input and the other for its output. Please refer to the
                         * following scenario to understand better:
                         * 
                         * > ls -al | grep read | grep readline.c
                         * 
                         * ls -al:
                         *      in: pipeB (unused) -> uses STDIN instead
                         *      out: pipeA
                         * 
                         * grep read
                         *      in: pipeA
                         *      out: pipeB
                         * 
                         * grep readline.c
                         *      in: pipeB
                         *      out: pipeA (unused) -> uses STDOUT instead
                         * 
                         * While two pipes are enough for it to work, we need to make sure that the in and out pipes are swapped for each command. This is the purpose
                         * of pipeCount. If it is odd, then the case will be the same as "ls -al" or "grep readline.c" in the scenario above. If it is even, the case
                         * will be the same as "grep read".
                         */
                        if (pipeCount % 2 == 0)
                        {
                            if (pipe(pipeA) < 0)
                            {
                                fprintf(stderr, "pipe A creation failed\n");
                            }
                        }
                        else
                        {
                            if (pipe(pipeB) < 0)
                            {
                                fprintf(stderr, "pipe B creation failed\n");
                            }
                        }

                        if (readFromPipe) // Prefixed by '|'
                            pipeState = PIP_BOTH;
                        else
                            pipeState = PIP_WRITE;

                        executeCommand(binPath, subArgC, cmd, __environ, BIN_FG, pipeState, getPipe, givePipe, RED_NONE, NULL, proDes);

                        if (subArgC + 1 < argCount)
                        {
                            parseCommand(&(cmd[subArgC + 1]), paths, 1, pipeCount + 1, proDes); // Since it is "|", it pipes the next command
                        }
                        break;
                    }
                    else if (strcmp(cmd[subArgC], ">") == 0)
                    {
                        // The current command is going to send its output to a file
                        piping = 1;

                        if (readFromPipe) // Prefixed by '|'
                            pipeState = PIP_BOTH;
                        else
                            pipeState = PIP_WRITE;

                        int redirState;

                        int outFilePos = subArgC + 1;

                        // If the operator is '>>' instead of '>'
                        if (strcmp(cmd[outFilePos], ">") == 0)
                        {
                            outFilePos++;
                            // If the operator is '>>>' or any weird stuff like that
                            if (isOperator(cmd[outFilePos][0]))
                            {
                                printf("%s: syntax error near unexpected token `%c'\n", SHELL_NAME, cmd[outFilePos][0]);
                                free(binPath);
                                return ERROR_SIG;
                            }
                            redirState = RED_APPE;
                        }
                        else
                        {
                            redirState = RED_OVER;
                        }
                        executeCommand(binPath, subArgC, cmd, __environ, BIN_FG, pipeState, getPipe, givePipe, redirState, cmd[outFilePos], proDes);

                        if (outFilePos + 2 < argCount)
                        {
                            parseCommand(&(cmd[outFilePos + 2]), paths, 0, pipeCount, proDes); // Since it is "|", it pipes the next command
                        }
                        break;
                    }
                }

                if (binState == BIN_FG && !piping)
                {
                    if (DEBUG)
                    {
                        printf("%d pipes were found in the command line\n", pipeCount);
                    }

                    if (readFromPipe)
                        pipeState = PIP_READ;
                    else
                        pipeState = PIP_NONE;

                    executeCommand(binPath, subArgC, cmd, __environ, binState, pipeState, getPipe, givePipe, RED_NONE, NULL, proDes);
                }
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
 *  path:       The complete path of the binary to be launched
 *  argc:       The number of arguments
 *  argv:       An array containing all the arguments
 *  envp:       An array containing the environment variables
 *  state:      The state in which the binary has to be launched (BIN_FG to launch normally or BIN_BG to launch it in the background)
 *  pipeState:  Determines the IO actions of the command (Refer to the enumerations at the top of this file for further information)
 *  getPipe:    The pipe from which the command will potentially read (getPipe > STDIN)
 *  givePipe:   The pipe to which the command will eventually write (STDOUT > givePipe)
 *  redirState: Determines if and how the program outputs goes to a file (Refer to the enumerations at the top for further information)
 *  outFile:    The name of file in which the command output will be redirected (NULL if the command output is not redirected to a file)
 *
 *  Returns: 0 if all went well
 */
int executeCommand(char *path, int argc, char **argv, char **envp, int state, int pipeState, int getPipe[2], int givePipe[2], int redirState, char *outFile, pProgDesc_t proDes)
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

        // TODO: These switches are a mess. Please find the resolve to make them organized, handsome and commented as well.
        switch (pipeState)
        {
        case PIP_NONE: // Does not use any pipe
            close(getPipe[WRITE_END]);
            close(getPipe[READ_END]);

            close(givePipe[WRITE_END]);
            close(givePipe[READ_END]);
            break;
        case PIP_WRITE:
            if (redirState != RED_NONE)
            {
                if (redirState == RED_OVER)
                    freopen(outFile, "w", stdout);
                else
                    freopen(outFile, "a+", stdout);
            }
            else
            {
                dup2(givePipe[WRITE_END], STDOUT_FILENO);
            }
            close(givePipe[READ_END]);
            break;
        case PIP_READ:
            dup2(getPipe[READ_END], STDIN_FILENO);
            close(getPipe[WRITE_END]);
            break;
        case PIP_BOTH:
            dup2(getPipe[READ_END], STDIN_FILENO);
            if (redirState != RED_NONE)
            {
                if (redirState == RED_OVER)
                    freopen(outFile, "w", stdout);
                else
                    freopen(outFile, "a+", stdout);
            }
            else
            {
                dup2(givePipe[WRITE_END], STDOUT_FILENO);
            }

            close(givePipe[READ_END]);
            break;
        default:
            fprintf(stderr, "Unknown pipe state");
            exit(-1);
            break;
        }

        execve(path, argvCpy, envp);
        perror("execve failed");
        exit(EXIT_FAILURE);
        break;
    default: // The current process is the parent
        if (DEBUG)
            printf("Is that you [%s] %d? Your father is right here kiddo!\n", path, childPid);

        // If the child process has been executed normally, waits for it to terminate
        if (state == BIN_FG)
        {
            switch (pipeState)
            {
            case PIP_NONE: // This command did not involve any pipes
                close(getPipe[WRITE_END]);
                close(getPipe[READ_END]);

                close(givePipe[WRITE_END]);
                close(givePipe[READ_END]);
                break;
            case PIP_WRITE: // This command is the first to be piped
                close(givePipe[WRITE_END]);
                break;
            case PIP_READ: // The executed command was the last to be piped
                close(getPipe[READ_END]);
                break;
            case PIP_BOTH:
                close(getPipe[READ_END]);

                close(givePipe[WRITE_END]);
                break;
            default:
                fprintf(stderr, "Unknown pipe state");
                exit(-1);
                break;
            }
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
 * Function: isOperator
 * --------------------
 * Determines whether or not the given character corresponds to one of the Shell operators
 * 
 *  c:       The character to test
 * 
 *  Returns: 1 if c is a Shell operator, 0 otherwise
 */
int isOperator(char c)
{
    for (int i = 0; i < SHELL_OPE_COUNT; i++)
        if (SHELL_OPE[i] == c)
            return 1;

    return 0;
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
 *
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