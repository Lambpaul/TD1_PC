
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>

#include "./readline.h"

/*
 * Read a line from standard input into a newly allocated 
 * array of char. The allocation is via malloc(size_t), the array 
 * must be freed via free(void*).
 */

char* readline(void) {
  #define BUFFER_LENGTH 256
  static char buffer[BUFFER_LENGTH];
  int offset = 0;
  for (;;) {
    char c = fgetc(stdin);
    if (c==EOF) {
      printf("PANIC: EOF on stdin\n");
      exit(-1);
    }
    if (c=='\n')
      break;
    buffer[offset++] = c;
  }
  buffer[offset++] = '\0';
  char *line = malloc(offset);
  strcpy(line,buffer);
  return line;
}

/* 
 * Split the string in words, according to the simple shell grammar.
 * Returns a null-terminated array of words.
 * The array has been allocated by malloc, it must be freed by free.
 */
char** split_in_words(char *line) {
  #define MAX_NWORDS 256
  static char* words[MAX_NWORDS];
  int nwords=0;
	char *cur = line;
	char c;
  words[0]=NULL;
	while ((c = *cur) != 0) {
		char *word = NULL;
		char *start;
		switch (c) {
		case ' ':
		case '\t':
			/* Ignore any whitespace */
			cur++;
			break;
		case '<':
			word = "<";
			cur++;
			break;
		case '>':
			word = ">";
			cur++;
			break;
		case '|':
			word = "|";
			cur++;
			break;
		case ';':
			word = ";";
			cur++;
			break;
		case '&':
			word = "&";
			cur++;
			break;
		default:
			/* Another word */
			start = cur;
      if (c=='"') {
        c = *++cur;
        while (c!='"')
          c = *++cur;
        cur++;
      } else {
        while (c) {
          c = *++cur;
          switch (c) {
          case 0:
          case ' ':
          case '\t':
          case '<':
          case '>':
          case '|':
          case ';':
          case '&':
            c = 0;
            break;
          default: ;
          }
        }
      }
			word = malloc((cur - start + 1) * sizeof(char));
			strncpy(word, start, cur - start);
			word[cur - start] = 0;
		}
		if (word) {
			words[nwords++] = word;
      words[nwords]=NULL;
		}
	}
  size_t size = (nwords + 1) * sizeof(char *);
  char** tmp = malloc(size);
  memcpy(tmp,words,size);
	return tmp;
}



/*int main(int argc, char** argv, char**envp) {

  for (int i=0;envp[i]!=NULL;i++)
    printf("env[%d]=%s\n",i,envp[i]);
  printf("\n");

  // set stdout without buffering so what is printed
  // is printed immediately on the screen.
  // setvbuf(stdout, NULL, _IONBF, 0); 
  // setbuf(stdout, NULL);
  
  for (;;) {
    printf("> ");
    fflush(stdout);
    char* line = readline();
    printf("%s\n", line);
    char** words = split_in_words(line);
    for (int i=0;words[i]!=NULL;i++)
      printf("[%s], ", words[i]);
    printf("\n");
    free(words);
    free(line);
  }
  return 0;
  
}*/