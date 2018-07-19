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

#define SERA_PORT 21122
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
  struct sockaddr_in hints, serA;
  int sockfd;  // serverA's socket
  // define the buffer used to load items from communication
  char buf[MAXBUFLEN];
  int numbytes;

  // create socket and decide whether it's successfully created
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if(sockfd == -1){
	perror("create serverA socket");
  }
  
  // set socket descriptor for AWS and serverA
  memset(&hints, 0, sizeof(hints));
  hints.sin_family = AF_INET;
  hints.sin_port = htons(AWS_PORT);
  memset(&serA, 0, sizeof(serA));
  serA.sin_family = AF_INET;
  serA.sin_port = htons(SERA_PORT);

  // start to bind the port number
  if(bind(sockfd, (struct sockaddr*)&serA, sizeof(serA))==-1){
	perror("serverA socket bind");
  }
  // print when booting up
  printf("The ServerA is up and running UDP on port %d.\n", SERA_PORT);
  
  // communicate with AWS
  while(1){
	// get word from AWS
	char word_str[MAXBUFLEN];
	int hints_len = sizeof(struct sockaddr);
	if((numbytes=recvfrom(sockfd, word_str, MAXBUFLEN-1, 0, (struct sockaddr*)&hints, (socklen_t*)&hints_len))==-1){
		perror("serverA receive <input> from AWS");
		break;
	}
	word_str[numbytes] = '\0';
	// get function from AWS
	char func_str[MAXBUFLEN];
        int serA_len = sizeof(struct sockaddr);
	if((numbytes=recvfrom(sockfd, func_str, MAXBUFLEN-1, 0, (struct sockaddr*)&serA, (socklen_t*)&serA_len))==-1){
		perror("serverA receive <function> from AWS");
		break;
	}
	func_str[numbytes] = '\0';

	// print <word> and <function> on the screen received from AWS
	printf("The ServerA received input < %s > and operation < %s >.\n", word_str, func_str);

  	/* do operation based on function received  */
	char c1[] = "search";
   	char c2[] = "prefix";
	char c3[] = "suffix";
	
  	if(strcmp(c1, func_str) == 0){
		FILE *fp;
		char StrLine[1024];  // used to read txt line by line
		char const *filename = "backendA.txt";
		int count = 0;  // used to count the number of matches
		int sim = 0; // used to count the # of similar words
		// use a specially interval array to load the <search> result to the AWS
		char findA[5000];
		fp = fopen(filename, "r");
		if (fp == NULL){
			printf("Could not open files %s", filename);
			return 1;
		}
		while(fgets(StrLine, 1024, fp) != NULL){
			char *r = strtok(StrLine, " :: ");  // points to word
  			char *def = strtok(NULL, "::");	// points to definition
			if(strcasecmp(r, word_str) == 0){  // 0: match 1: similar
				count++;
				strcat(findA, "match");
				strcat(findA, ",,");
				strcat(findA, r);
				strcat(findA, "++");
				strcat(findA, def);
				strcat(findA, "::");
				//printf("%s\n", r);
			}else if(isEqual(word_str, r)){
				sim++;
				strcat(findA, "similar");
				strcat(findA, ",,");
				strcat(findA, r);
				strcat(findA, "++");
				strcat(findA, def);
				strcat(findA, "::");
				//printf("%s\n", r);
			}
		}
		//printf("Transmitted data: %s\n", findA);
		printf("The serverA has found <%d> matches and <%d> similar words\n", count, sim);
		// send search results to AWS
		if(sendto(sockfd, findA, strlen(findA), 0, (struct sockaddr*)&hints, sizeof(struct sockaddr)) == -1){
			perror("serverA send search results to AWS");
			break;
		}
		printf("The ServerA finished sending the output to AWS\n");
		//fclose(fp);
	}else if(strcmp(c2, func_str) == 0){
		FILE *fp;
		char StrLine[1024];  // used to read txt line by line
		char const *filename = "backendA.txt";
		int count = 0;  // used to count the number of matches
		// an array used to load word list
		char preA[MAXBUFLEN];
		fp = fopen(filename, "r");
		if (fp == NULL){
			printf("Could not open files %s", filename);
			return 1;
		}
		while(fgets(StrLine, 1024, fp) != NULL){
			char *r = strtok(StrLine, " :: ");  // points to word
  			char *def = strtok(NULL, "::");	// points to definition
			if(isPrefix(word_str, r)){
				count++;
				strcat(preA, r);
				strcat(preA, "::");
				//printf("word:%s\n", r);
				//printf("Definition:%s\n", def);
			}
		}
		printf("The ServerA has found < %d > matches\n", count);

		// Send word list to AWS after executing <prefix>
		if(sendto(sockfd, preA, strlen(preA), 0, (struct sockaddr*)&hints, sizeof(struct sockaddr)) == -1){
			perror("serverA send prefix results to AWS");
			break;
		}
		printf("The ServerA finished sending the output to AWS\n");
	}else if(strcmp(c3, func_str) == 0){
		
	}else{
		printf("Please input <search>, <prefix> or <suffix>!\n");
	}
	
  }
  close(sockfd);
  return 0;
}
