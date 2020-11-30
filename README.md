## QuYsh Shell

@Authors:
    Corentin HUMBERT
    Paul LAMBERT

The Shell code can be found in file titled "quysh.c"

Logs:

    Version 1.0.0 (Beyond-Grub):
        + Removed native print command
        + Implemented environment variable as command arguments:
            - The equivalent of "print VAR" is now "echo $VAR"

    Version 0.99.1 (File Revolution):
        + Fixed bugs with '>' and '>>':
            - '>>>', '>>&' or '>>|' will now output an error
            - '>' was not working for processes reading from a pipe (ex: ls -al | grep readline > toto)
        + Added a function to check if a character is a Shell operator or not
        + Refactored code
        + Added additional DEBUG prints

    Version 0.99 (File Revolution) [UNSTABLE]:
        + Implemented file redirections through support of '>' and '>>' operators
        + TODO: Refactor code (pipes and redirections)
        + TODO: Solve bugs [Shell is UNSTABLE and can be broken]
        + TODO: Write comments and update function comments of already written function

    Version 0.97.3 (Pied Piper):
        + Removed uncessary lines
        + TODO: Fix '|' and '&' for commands "cd", "print" and "set"
        + TODO: Shell is not recursive for "cd", "print" and "set"
    
    Version 0.97.2 (Pied Piper):
        + Implemented a string replace of '~' by content of $HOME
        + Started working on fixing '|' and '&' for commands "cd", "print" and "set"
        + TODO: Shell is not recursive for "cd", "print" and "set"
        
    Version 0.97.1 (Piped Piper):
        + Started commenting pipes
        + Added missing comments for previous features

    Version 0.97 (Pied Piper):
        + Fully implemented pipes (PITA)

    Version 0.9:
        + Implemented pipes for two chained commands

    Version 0.8:
        + Fixed a bug with execve
        + Added Program Descriptor
            - A structure that keeps tracks of all children programs running in background
        + Added comments