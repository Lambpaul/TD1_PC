## QuYsh Shell

@Authors:
    Corentin HUMBERT
    Paul LAMBERT

Logs:

    Version 0.99:
        + Implemented file redirections through support of '>' and '>>' operators
        + TODO: Refactor code (pipes and redirections)
        + TODO: Solve bugs [Shell is UNSTABLE and can be broken]
        + TODO: Write comments and update function comments of already written function

    Version 0.97.3:
        + Removed uncessary lines
        + TODO: Fix '|' and '&' for commands "cd", "print" and "set"
        + TODO: Shell is not recursive for "cd", "print" and "set"
    
    Version 0.97.2:
        + Implemented a string replace of '~' by content of $HOME
        + Started working on fixing '|' and '&' for commands "cd", "print" and "set"
        + TODO: Shell is not recursive for "cd", "print" and "set"
        
    Version 0.97.1:
        + Started commenting pipes
        + Added missing comments for previous features

    Version 0.97:
        + Fully implemented pipes (PITA)

    Version 0.9:
        + Implemented pipes for two chained commands

    Version 0.8:
        + Fixed a bug with execve
        + Added Program Descriptor
            - A structure that keeps tracks of all children programs running in background
        + Added comments