
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char** argv, char**envp) {
  int pid_fils=fork();
  int status;
  if(pid_fils<0)
    perror("Erreur");
  else{
    if(pid_fils==0){
      printf("Fils\n");
    }else{
      int new_pid=wait(&status);
      printf("Father and my son who died is %d and was %d\n", new_pid, pid_fils);
    }
  }
  return 0;
}