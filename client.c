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

#define AWSPORT 25122 // the AWSport client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int main(int argc, char *argv[])
{
    // sockfd: socket descriptor
    int sockfd, numbytes;  
    char buf[MAXDATASIZE];

    // hints points to a struct addrinfo, servinfo points to results
    // addrinfo is address struct
    struct addrinfo hints, *servinfo, *p;
    int rv;
    // connection's address 
    char s[INET_ADDRSTRLEN];

    // decide whether the number of inputs is correct
    if (argc != 3) {
        printf("number of inputs is incorrect, it should be two: function and word\n");
        return 1;
    }
    // prepare address struct
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    // to make IP address stored in network byte order
    int htons_AWSPORT = htons(AWSPORT);
    int len = snprintf(NULL, 0, "%d", htons_AWSPORT);
    char htons_AWSPORT_char[len +1];
    sprintf(htons_AWSPORT_char, "%d", htons_AWSPORT);
    htons_AWSPORT_char[len] = '\0';

    if ((rv = getaddrinfo("127.0.0.1", htons_AWSPORT_char, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
	// create socket and decide whether it's successfully created
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
	{
            perror("client: socket");
            continue;
        }
	// connect to an IP address
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break; // successful socket creation and connection
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    printf("The client is up and running.\n");

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

    //printf("client: connecting to %s\n", s); no use

    freeaddrinfo(servinfo); // all done with this structure

    // set a char array to put <function> and <input>
    char newBuf1[strlen(argv[1])+strlen(argv[2])];
    // write formatted output into buffer
    snprintf(newBuf1, sizeof(newBuf1), "%s %s", argv[1], argv[2]);

    // pointer word points to <word>
    char *word = (char*) argv[2];

    // prepare to send <word> to AWS
    int send_bytes;
    if((send_bytes = send(sockfd, word, strlen(word), 0))==-1){
	perror("send inputs");
    }
    // use numbytes to receive and decide whether it's correct
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    // printf("client: received '%s'\n",buf);  not use

    // pointer func points to <function>
    char *func = (char*) argv[1];
    // prepare to send <function> to AWS
    if((send_bytes = send(sockfd, func, strlen(func), 0))==-1){
	perror("send function");
    }
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    buf[numbytes] = '\0';
    // last print of client in phase 1a
    printf("The client sent < %s > and < %s > to AWS.\n", word, func);
    // start to receive <search> and <prefix> results from AWS
    if(strcmp("search", func) == 0){
        if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1){
            perror("receive search result from AWS");
            exit(1);
        }
        if(numbytes == 0){
            printf("Found no matches for <%s>\n", word);
        }
        buf[numbytes] = '\0';
        //printf("Search result from AWS:%s\n", buf);
        printf("Found a match for <%s>:\n", word);
        char *r = strtok(buf, "++");
        while(r = strtok(NULL, "++")){
            printf("<%s>\n", r);
        }
    }else if(strcmp("prefix", func) == 0){
        // receive prefix results sent from AWS
        if((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1){
            perror("receive prefix result from AWS");
            exit(1);
        }
        buf[numbytes] = '\0';
        //printf("%s\n", buf);
        char differ[500];
        char *t = strtok(buf, "::");
        strcat(differ, t);
        strcat(differ, "::");
        char *temp;
        int count = 1;
        temp = t;
        while(t = strtok(NULL, "::")){
            if(strcasecmp(temp, t) == 0){
                continue;
            }else{
                count++;
                strcat(differ, t);
                strcat(differ, "::");
                temp = t;
            }
        }
        printf("Found < %d > matches for < %s >:\n", count, word);
        // start normal calculation after preprocessing
        char *q = strtok(differ, "::");
        printf("< %s >\n", q);
        while(q = strtok(NULL, "::")){
            printf("< %s >\n", q);
        }
    }

    close(sockfd);
    return 0;
}
