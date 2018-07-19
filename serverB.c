#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define SERB_PORT 22122
#define AWS_PORT 24122
#define MAXBUFLEN 100

/* decide whether similar, 0:similar, 1: not similar*/
int isEqual(char a[], char b[]){
  if(strlen(a)==strlen(b)){
    if((a[0] == b[0] + 32) || (a[0] == b[0] -32)){
      int count = 0;
      for(int i=1;i<strlen(a);i++){
        if(a[i]==b[i]){
          continue;
        }else if(count == 0){
          count++;
        }else{
          return 0;
          break;
        }
      }
      return 1;
    }else{
      int count = 0;
      for(int i=0;i<strlen(a);i++){
        if(a[i]==b[i]){
          continue;
        }else if(count == 0){
          count++;
        }else{
          return 0;
          break;
        }
      }
      return 1;
    }
  }else{
    return 0;
  }
}

// prefix decision 0: no, 1: yes
int isPrefix(char b[], char c[]){
  if(b[0]>='a'&&b[0]<='z'){
    b[0] = b[0] - 32;
  }
  if(strlen(c) >= strlen(b)){
    for(int i=0;i<strlen(b);i++){
      if(b[i] == c[i]){
        continue;
      }else{
        return 0;
        break;
      }
    }
    return 1;
  }else{
    return 0;
  }
}

int main(int argc, char const *argv[])
{
  // address struct
  struct sockaddr_in hints, serB;
  int sockfd;
  // define the buffer used to load items from communication
  char buf[MAXBUFLEN];
  int numbytes;

  // create socket and decide whether it's successfully created
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd == -1){
	perror("create serverB socket");
  }
  
  // set socket descriptor for AWS and serverA
  memset(&hints, 0, sizeof(hints));
  hints.sin_family = AF_INET;
  hints.sin_port = htons(AWS_PORT);
  memset(&serB, 0, sizeof(serB));
  serB.sin_family = AF_INET;
  serB.sin_port = htons(SERB_PORT);

  // start to bind the port number
  if(bind(sockfd, (struct sockaddr*)&serB, sizeof(serB))==-1){
	perror("serverB socket bind");
  }
  // print when booting up
  printf("The ServerB is up and running UDP on port %d.\n", SERB_PORT);
  
  // communicate with AWS
  while(1){
	// get word from AWS
	char word_str[MAXBUFLEN];
	int hints_len = sizeof(struct sockaddr);
	if((numbytes=recvfrom(sockfd, word_str, MAXBUFLEN-1, 0, (struct sockaddr*)&hints, (socklen_t*)&hints_len))==-1){
		perror("serverB recvfrom");
		break;
	}
	word_str[numbytes] = '\0';
	// get function from AWS
	char func_str[MAXBUFLEN];
        int serB_len = sizeof(struct sockaddr);
	if((numbytes=recvfrom(sockfd, func_str, MAXBUFLEN-1, 0, (struct sockaddr*)&serB, (socklen_t*)&serB_len))==-1){
		perror("serverB receive <function> from AWS");
		break;
	}
	func_str[numbytes] = '\0';

	// print <word> and <function> on the screen received from AWS
	printf("The ServerB received input < %s > and operation < %s >.\n", word_str, func_str);
  	/* do operation based on function received  */
  char c1[] = "search";
  char c2[] = "prefix";
  char c3[] = "suffix";
  
  if(strcmp(c1, func_str) == 0){
    FILE *fp;
    char StrLine[1024];  // used to read txt line by line
    char const *filename = "backendB.txt";
    int count = 0;  // used to count the number of matches
    int sim = 0; // used to count the # of similar words
    char findB[5000];
    fp = fopen(filename, "r");
    if (fp == NULL){
      printf("Could not open files %s", filename);
      return 1;
    }
    while(fgets(StrLine, 1024, fp) != NULL){
      char *r = strtok(StrLine, " :: ");  // points to word
      char *def = strtok(NULL, "::"); // points to definition
      if(strcasecmp(r, word_str) == 0){
        count++;
        strcat(findB, "match");
        strcat(findB, ",,");
        strcat(findB, r);
        strcat(findB, "++");
        strcat(findB, def);
        strcat(findB, "::");
        //printf("%s\n", r);
      }else if(isEqual(word_str, r)){
        sim++;
        strcat(findB, "similar");
        strcat(findB, ",,");
        strcat(findB, r);
        strcat(findB, "++");
        strcat(findB, def);
        strcat(findB, "::");
        //printf("%s\n", r);
      }
    }
    printf("The serverB has found <%d> matches and <%d> similar words\n", count, sim);
    // send search results to AWS
    if(sendto(sockfd, findB, strlen(findB), 0, (struct sockaddr*)&hints, sizeof(struct sockaddr)) == -1){
      perror("serverB send search results to AWS");
      break;
    }
    printf("The ServerB finished sending the output to AWS\n");
    //fclose(fp);
  }else if(strcmp(c2, func_str) == 0){
    FILE *fp;
    char StrLine[1024];  // used to read txt line by line
    char const *filename = "backendB.txt";
    int count = 0;  // used to count the number of matches
    // an array used to load word list
    char preB[MAXBUFLEN];
    //strcat(preA, "A:");
    fp = fopen(filename, "r");
    if (fp == NULL){
      printf("Could not open files %s", filename);
      return 1;
    }
    while(fgets(StrLine, 1024, fp) != NULL){
      char *r = strtok(StrLine, " :: ");  // points to word
      char *def = strtok(NULL, "::"); // points to definition
      if(isPrefix(word_str, r)){
        count++;
        strcat(preB, r);
        strcat(preB, "::");
        //printf("word:%s\n", r);
        //printf("Definition:%s\n", def);
      }
    }
    printf("The ServerB has found < %d > matches\n", count);

    // Send word list to AWS after executing <prefix>
    if(sendto(sockfd, preB, strlen(preB), 0, (struct sockaddr*)&hints, sizeof(struct sockaddr)) == -1){
      perror("serverB send prefix results to AWS");
      break;
    }
    printf("The ServerB finished sending the output to AWS\n");
  }else if(strcmp(c3, func_str) == 0){
    
  }else{
    printf("Please input <search>, <prefix> or <suffix>!\n");
  }
  
	
  }
  close(sockfd);
  return 0;
}
