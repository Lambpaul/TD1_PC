
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "./readline.h"

int main(int argc, char** argv, char**envp) {
  for (;;) {
    printf("> ");
    fflush(stdout);
    char* line = readline();
    printf("%s\n", line);
    char** words = split_in_words(line);
    for (int i=0;words[i]!=NULL;i++){
      printf("[%s], ", words[i]);
      
    }

    printf("\n");
    free(words);
    free(line);
  }
  return 0;
}