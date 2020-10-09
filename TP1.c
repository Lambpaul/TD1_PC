
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
    if(strcmp(words[0],"cd")==0){
        int err = chdir(words[1]);
        if(err==-1){
          printf("Erreur: impossible de se déplacer dans %s\n",words[1]);
        }else{
          printf("cd DONE.\n");
        }
    }
    else{
      if(strcmp(words[0],"pwd")==0){
        char s[100];
        printf("%s\n",getcwd(s,100));
      }
    }
    for (int i=0;words[i]!=NULL;i++){
      printf("[%s], ", words[i]);
      
    }

    printf("\n");
    free(words);
    free(line);
  }
  return 0;
}