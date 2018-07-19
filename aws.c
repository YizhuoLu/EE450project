#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define TCP_C_PORT 25122 // port for client
#define TCP_M_PORT 26122 // provide with TCP connection with monitor
#define UDP_PORT 24122 // AWS's UDF port number
#define SERA_PORT 21122 // for serverA
#define SERB_PORT 22122 // for serverB
#define SERC_PORT 23122 // for serverC
#define BACKLOG 20 // pending connections queue
#define MAXRECV 100


void sigchld_handler(int s)
{
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// 取得 sockaddr，IPv4 或 IPv6：
void *get_in_addr(struct sockaddr *sa)
{
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{
  // client socket and addr preparation
  int sockfd, new_fd; // 在 sock_fd 进行 listen，new_fd 是新的连接  //sockfd is client socket
  struct addrinfo hints, *servinfo, *p; 
  struct sockaddr_storage their_addr; // 连接者的地址资料 others trying to connect, their_addr used to connect client
  socklen_t sin_size;
  struct sigaction sa;

  // to reuse port
  int yes=1;
  char s[INET_ADDRSTRLEN];
  int rv, numbytes;

  // prepare sockets for serverA, serverB, serverC
  struct sockaddr_in aws_addr, serA_addr, serB_addr, serC_addr;
  int sock_serA, sock_serB, sock_serC;
  
  // beej
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // 使用我的 IP

  // to make IP address stored in network byte order   for client
  int htons_TCP_C_PORT = htons(TCP_C_PORT);
  int len = snprintf(NULL, 0, "%d", htons_TCP_C_PORT);

  char htons_TCP_C_PORT_char[len +1];
  sprintf(htons_TCP_C_PORT_char, "%d", htons_TCP_C_PORT);
  htons_TCP_C_PORT_char[len] = '\0';

  if ((rv = getaddrinfo("127.0.0.1", htons_TCP_C_PORT_char, &hints, &servinfo)) != 0) {
    perror("server AWS getaddrinfo client");
    return 1;
  }

  // 以循环找出全部的结果，并绑定（bind）到第一个能用的结果
  for(p = servinfo; p != NULL; p = p->ai_next) {
  // create a socket and decide whethere it's created successfully
    	if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      		perror("server: socket");
      		continue;  // continue to next addrinfo
    }  

  

  // permit port reusing
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      	perror("setsockopt");
      	exit(1);
    }
  /* end of preparation for client */

                                                                      // insert parts of preparing monitor


    // bind to port
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    printf("failure in creating socket and binding ports.\n");
    return 2;
  }

  // listen
  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // kill died processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }
  // print booting up message
  printf("The AWS is up and running.\n");
  
                         /*   load up aws address struct for UDP connection       */
  memset(&aws_addr, 0, sizeof(aws_addr));
  aws_addr.sin_family = AF_INET;
  aws_addr.sin_port = htons(UDP_PORT);

             /*               set up ServerA                    */
  // prepare ServerA socket
  sock_serA = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sock_serA == -1){
	perror("AWS ServerA: socket");
  }

  memset(&serA_addr, 0, sizeof(serA_addr));
  serA_addr.sin_family = AF_INET;
  serA_addr.sin_port = htons(SERA_PORT);

	 	/* set up ServerB */
  // prepare ServerB socket
  sock_serB = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sock_serB == -1){
	perror("AWS ServerB: socket");
  }

  memset(&serB_addr, 0, sizeof(serB_addr));
  serB_addr.sin_family = AF_INET;
  serB_addr.sin_port = htons(SERB_PORT);

		/* set up ServerC */
  // prepare ServerC socket
  sock_serC = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(sock_serC == -1){
	perror("AWS ServerC: socket");
  }

  memset(&serC_addr, 0, sizeof(serC_addr));
  serC_addr.sin_family = AF_INET;
  serC_addr.sin_port = htons(SERC_PORT);

                                        /*          monitor connect         */
  	/*int sock_monitor, new_fd_M;
  	struct addrinfo hints_M, *servinfo_M, *p_M;
  	struct sockaddr_storage their_addrM; 
  	socklen_t sin_size_M; 
  	int rv_M; 
  	char M[INET_ADDRSTRLEN];

  	memset(&hints_M, 0, sizeof hints_M);
  	hints_M.ai_family = AF_UNSPEC;
  	hints_M.ai_socktype = SOCK_STREAM;

  	int htons_TCP_M_PORT = htons(TCP_M_PORT);
  	len = snprintf(NULL, 0, "%d", htons_TCP_M_PORT);
 	char htons_TCP_M_PORT_char[len +1];
  	sprintf(htons_TCP_M_PORT_char, "%d", htons_TCP_M_PORT);
  	htons_TCP_M_PORT_char[len] = '\0';

  	if ((rv_M = getaddrinfo("127.0.0.1", htons_TCP_M_PORT_char, &hints_M, &servinfo_M)) != 0) {
    	perror("server AWS getaddrinfo monitor");
    	return 1;
  	}

    // for monitor
  	for(p_M = servinfo_M; p_M != NULL; p_M = p_M->ai_next) {
    	// create a socket and decide whethere it's created successfully
    	if ((sock_monitor = socket(p_M->ai_family, p_M->ai_socktype, p_M->ai_protocol)) == -1) {
      		perror("server: socket for monitor");
      		continue;  // continue to next addrinfo
    	}

   		// permit port reusing
    	if (setsockopt(sock_monitor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      		perror("setsockopt");
      		exit(1);
    	}

    	// bind to port
    	if (bind(sock_monitor, p_M->ai_addr, p_M->ai_addrlen) == -1) {
      		close(sock_monitor);
      		perror("server: bind to monitor");
      		continue;
    	}
    	break;
    }

  	if (p_M == NULL) {
    	printf("failure in creating socket and binding ports with monitor.\n");
    	return 2;
  	}

  	freeaddrinfo(servinfo_M); // all in this structure

	// listen monitor
  	if (listen(sock_monitor, BACKLOG) == -1) {
    	perror("listen to monitor");
    	exit(1);
  	}
  	sin_size_M = sizeof their_addrM;
    new_fd_M = accept(sock_monitor, (struct sockaddr *)&their_addrM, &sin_size_M);

    if (new_fd_M == -1) {
      perror("accept monitor");
      exit(1);
    }

    inet_ntop(their_addrM.ss_family, get_in_addr((struct sockaddr *)&their_addrM), M, sizeof M);*/
  										/*          monitor END                       */
  while(1) { // main accept loop
  						/*     begin accepting available connections from client   */
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
    int send_bytes;
    // receive <word>
    char item[MAXRECV];
    if((numbytes = recv(new_fd, item, MAXRECV-1, 0))==-1){
	perror("receive input");
    	exit(1);
    }
    item[numbytes] = '\0';
    // ACK for receiving <word>
    char const *ackWord = "Got word";
    if((send_bytes = send(new_fd, ackWord, strlen(ackWord), 0))==-1){
	perror("send ack input");
    }

    // receive <function>
    char func[MAXRECV];
    if((numbytes = recv(new_fd, func, MAXRECV-1, 0))==-1){
	perror("receive function");
    	exit(1);
    }
    func[numbytes] = '\0';
    // ACK for receiving <function>
    char const *ackFun = "Got function";
    if((send_bytes = send(new_fd, ackFun, strlen(ackFun), 0))==-1){
	perror("send ack function");
    }
    // print receive message to the screen
    printf("The AWS received input=< %s > and function=< %s > from the client using TCP over port < %d >.\n", item, func, TCP_C_PORT);

    /* AWS begin to send <function> and <input> to three backend servers*/
    // send <input> to serverA
    char *wordToSerA = item;
    if(sendto(sock_serA, wordToSerA, strlen(wordToSerA), 0, (struct sockaddr*)&serA_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <input> to ServerA");
	break;
    }
    // send <function> to serverA
     char *funcToSerA = func;
     if(sendto(sock_serA, funcToSerA, strlen(funcToSerA), 0, (struct sockaddr*)&serA_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <function> to ServerA");
	break;
    }
    printf("The AWS sent < %s > and < %s > to Backend-Server A.\n", wordToSerA, funcToSerA);

    // send <input> to serverB
    char *wordToSerB = item;
    if(sendto(sock_serB, wordToSerB, strlen(wordToSerB), 0, (struct sockaddr*)&serB_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <input> to ServerB");
	break;
    }
    // send <function> to serverB
     char *funcToSerB = func;
     if(sendto(sock_serB, funcToSerB, strlen(funcToSerB), 0, (struct sockaddr*)&serB_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <function> to ServerB");
	break;
    }
    printf("The AWS sent < %s > and < %s > to Backend-Server B.\n", wordToSerB, funcToSerB);

    // send <input> to serverC
    char *wordToSerC = item;
    if(sendto(sock_serC, wordToSerC, strlen(wordToSerC), 0, (struct sockaddr*)&serC_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <input> to ServerC");
	break;
    }
    // send <function> to serverC
     char *funcToSerC = func;
     if(sendto(sock_serC, funcToSerC, strlen(funcToSerC), 0, (struct sockaddr*)&serC_addr, sizeof(struct sockaddr)) == -1){
	perror("AWS send <function> to ServerC");
	break;
    }
    printf("The AWS sent < %s > and < %s > to Backend-Server C.\n", wordToSerC, funcToSerC);

                            /*    This part is used to receive results based on what functions is sent by client          */
   	if(strcmp("prefix", func) == 0){
   		 // Receive prefix result from serverA
    	char recA[MAXRECV];
   		socklen_t serA_addr_len = sizeof (struct sockaddr);

    	if((numbytes = recvfrom(sock_serA, recA, sizeof(recA), 0, (struct sockaddr*)&serA_addr, &serA_addr_len)) == -1){
    		perror("recvfrom serverA");
    		break;
    	}
    	recA[MAXRECV] = '\0';
    	int count = 0; // used to count the number of prefix words
    	char *r = strtok(recA, "::");
    	//printf("%s\n", r);
    	//count++;
    	while(r = strtok(NULL, "::")){
    		count++;
    		//printf("%s\n", r);
   	 	}
   	 	count++;
                         /*      prepare prefix results and send to client         */
   	 	char preLoad[500];
   	 	strcat(preLoad, recA);
   	 	strcat(preLoad, "::");
     	int total = count;  // total matches count
    	printf("The AWS received <%d> matches from Backend-Server<A> using UDP over port <%d>\n",count, SERA_PORT );
    	// clean up <count> and <preA>
    	count = 0;
    	memset(&recA, 0, sizeof recA);

    	// Receive prefix result from serverB
    	char recB[MAXRECV];
    	socklen_t serB_addr_len = sizeof (struct sockaddr);

    	if((numbytes = recvfrom(sock_serB, recB, sizeof(recB), 0, (struct sockaddr*)&serB_addr, &serB_addr_len)) == -1){
    		perror("recvfrom serverB");
    		break;
    	}
    	recB[MAXRECV] = '\0';
    	//printf("%s", recA);
    	int countB = 0; // used to count the number of prefix words
    	char *r1 = strtok(recB, "::");
    	//printf("%s\n", r1);
    	//countB++;
    	while(r1 = strtok(NULL, "::")){
    		countB++;
    		//printf("%s\n", r1);
    	}
    	countB++;
   			 /*      prepare prefix results and send to client         */

    	strcat(preLoad, recB);
    	strcat(preLoad, "::");
    	total += countB;
    	printf("The AWS received <%d> matches from Backend-Server<B> using UDP over port <%d>\n",countB, SERB_PORT );
    	// clean up <count> and <preA>
   		countB = 0;
    	memset(&recB, 0, sizeof recB);

    	// Receive prefix result from serverC
    	char recC[MAXRECV];
    	socklen_t serC_addr_len = sizeof (struct sockaddr);

    	if((numbytes = recvfrom(sock_serC, recC, sizeof(recC), 0, (struct sockaddr*)&serC_addr, &serC_addr_len)) == -1){
    		perror("recvfrom serverC");
    		break;
    	}
    	recC[MAXRECV] = '\0';
    	int countC = 0; // used to count the number of prefix words
    	char *r2 = strtok(recC, "::");
    	//printf("%s\n", r2);
    	//countC++;
    	while(r2 = strtok(NULL, "::")){
    		countC++;
    		//printf("%s\n", r2);
    	}
    	countC++;
                         /*      prepare prefix results and send to client         */

    	strcat(preLoad, recC);
    	strcat(preLoad, "::");
    	total += countC;
    	printf("The AWS received <%d> matches from Backend-Server<C> using UDP over port <%d>\n",countC, SERC_PORT );
    	// clean up <countC> and <preC>
    	countC = 0;
    	memset(&recC, 0, sizeof recC);
    	if((send_bytes = send(new_fd, preLoad, strlen(preLoad), 0)) == -1){
    		perror("send prefix results to client");
    	}
    	//printf("%s\n", preLoad);
    	printf("The AWS sent < %d > matches to client.\n", total);

    	                          /*    send prefix results to monitor             */

    	/*if((send_bytes = send(new_fd_M, item, strlen(item), 0)) == -1){
    		perror("send <input> to monitor");
    	}
    	if((send_bytes = send(new_fd_M, func, strlen(func), 0)) == -1){
    		perror("send <function> to monitor");
    	}
    	if((send_bytes = send(new_fd_M, preLoad, strlen(preLoad), 0)) == -1){
    		perror("send prefix results to monitor");
    	}
    	printf("The AWS sent < %d > matches to the monitor via TCP port < %d >.\n", total, TCP_M_PORT);*/
    	total = 0;
    	memset(&preLoad, 0, sizeof preLoad);
   	}else if(strcmp("search", func) == 0){
   		/*      Receive <search> results from serverA          */
   		char dataA[10000];
   		int simA = 0; // used to count the number of similar words
   		socklen_t serA_addr_len = sizeof (struct sockaddr);
   		if((numbytes = recvfrom(sock_serA, dataA, sizeof dataA, 0, (struct sockaddr*)&serA_addr, &serA_addr_len)) == -1){
   			perror("recvfrom serverA of search");
   			break;
   		}
   		dataA[10000] = '\0';
   		// apart
   		// array used to send match word and its definition to client, array sim used to send similar word and its def to monitor
   		char mat[2500];
   		//matA[0] = '0';
   		char sim[500];
   		char temp[500];
   		int sigM = 0;  // used to signify whether array mat[] has been loaded
   		int sigS = 0;  // used to signify whether array sim[] has been loaded
   		//printf("result:%s\n", dataA);
   		char *rA = strtok(dataA, "::");
   		for(int i=0;i<strlen(rA);i++){
   			if(rA[i] == ','){
   				for(int k=0;k<(i-1);k++){
   					temp[k] = rA[k];
   				}
   				if(strcmp(temp, "similar") == 0){
   					simA++;
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rA);k++){
   						sim[a++] = rA[k-1];
   					}
   					//printf("%s\n", sim);
   					sigS = 1;
   					break;
   				}else if(strcmp(temp, "match") == 0){
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rA);k++){
   						mat[a++] = rA[k-1];
   					}
   					//printf("%s\n", matA);

   					sigM = 1;
   					break;
   				}
   			}
   		}
   		while(rA = strtok(NULL, "::")){
   			for(int i=0;i<strlen(rA);i++){
   				if(rA[i] == ','){
   					for(int k=0;k<(i-1);k++){
   						temp[k] = rA[k];
   					}
   					if(strcmp(temp, "similar") == 0){
   						if(sigS == 1){
   							break;
   						}else{
   							simA++;
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rA);k++){
   								sim[a++] = rA[k];
   							}
   							//printf("%s\n", sim);
   							sigS = 1;
   							break;
   						}
   					}else if(strcmp(temp, "match") == 0){
   						if(sigM == 1){
   							break;
   						}else{
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rA);k++){
   								mat[a++] = rA[k];
   							}
   							//printf("%s\n", mat);
   							sigM = 1;
   							break;
   						}	
   					}
   				}
   			}
   		}
   		//printf("%s\n", sim);
   		//printf("%s\n", mat);
   		if(simA>0){
   			printf("The AWS received < 1 > similar words from Backend-Server < A > using UDP over port < %d >\n", SERA_PORT);
   		}else{
   			printf("The AWS received < 0 > similar words from Backend-Server < A > using UDP over port < %d >\n", SERA_PORT);
   		}
   		printf("%s\n", mat);
   		
   		// clean up char array and marks
   		sigS = 0;
   		sigM = 0;
   		memset(&temp, 0, sizeof temp);
   		//memset(&sim, 0, sizeof sim);
   		memset(&dataA, 0, sizeof dataA);
   		simA = 0;
   		/*              end of receiving search results from serverA                */
   		/*              Receive <search> results from serverB                */
   		char dataB[10000];
   		int simB = 0; // used to count the number of similar words
   		socklen_t serB_addr_len = sizeof (struct sockaddr);
   		if((numbytes = recvfrom(sock_serB, dataB, sizeof dataB, 0, (struct sockaddr*)&serB_addr, &serB_addr_len)) == -1){
   			perror("recvfrom serverB of search");
   			break;
   		}
   		dataB[10000] = '\0';
   		// apart
   		char *rB = strtok(dataB, "::");
   		for(int i=0;i<strlen(rB);i++){
   			if(rB[i] == ','){
   				for(int k=0;k<(i-1);k++){
   					temp[k] = rB[k];
   				}
   				if(strcmp(temp, "similar") == 0){
   					simB++;
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rB);k++){
   						sim[a++] = rB[k-1];
   					}
   					//printf("%s\n", sim);
   					sigS = 1;
   					break;
   				}else if(strcmp(temp, "match") == 0){
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rB);k++){
   						mat[a++] = rB[k-1];
   					}
   					sigM = 1;
   					break;
   				}
   			}
   		}
   		while(rB = strtok(NULL, "::")){
   			for(int i=0;i<strlen(rB);i++){
   				if(rB[i] == ','){
   					for(int k=0;k<(i-1);k++){
   						temp[k] = rB[k];
   					}
   					if(strcmp(temp, "similar") == 0){
   						if(sigS == 1){
   							break;
   						}else{
   							simB++;
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rB);k++){
   								sim[a++] = rB[k];
   							}
   							//printf("%s\n", sim);
   							sigS = 1;
   							break;
   						}
   					}else if(strcmp(temp, "match") == 0){
   						if(sigM == 1){
   							break;
   						}else{
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rB);k++){
   								mat[a++] = rB[k];
   							}
   							sigM = 1;
   							break;
   						}	
   					}
   				}
   			}
   		}
   		
   		if(simB>0){
   			printf("The AWS received < 1 > similar words from Backend-Server < B > using UDP over port < %d >\n", SERB_PORT);
   		}else{
   			printf("The AWS received < 0 > similar words from Backend-Server < B > using UDP over port < %d >\n", SERB_PORT);
   		}
   		sigS = 0;
   		sigM = 0;
   		memset(&temp, 0, sizeof temp);
   		//memset(&sim, 0, sizeof sim);
   		memset(&dataB, 0, sizeof dataB);
   		simB = 0;
   		/*  End of receiving search result from serverB        */
   		/*              Receive <search> results from serverC                */
   		char dataC[10000];
   		int simC = 0; // used to count the number of similar words
   		socklen_t serC_addr_len = sizeof (struct sockaddr);
   		if((numbytes = recvfrom(sock_serC, dataC, sizeof dataC, 0, (struct sockaddr*)&serC_addr, &serC_addr_len)) == -1){
   			perror("recvfrom serverC of search");
   			break;
   		}
   		dataC[10000] = '\0';
   		//printf("Data from C:%s\n", dataC);
   		// apart
   		char *rC = strtok(dataC, "::");
   		for(int i=0;i<strlen(rC);i++){
   			if(rC[i] == ','){
   				for(int k=0;k<(i-1);k++){
   					temp[k] = rC[k];
   				}
   				if(strcmp(temp, "similar") == 0){
   					simC++;
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rC);k++){
   						sim[a++] = rC[k-1];
   					}
   					//printf("%s\n", sim);
   					sigS = 1;
   					break;
   				}else if(strcmp(temp, "match") == 0){
   					i+=2;
   					int a = 0;
   					for(int k=i;k<strlen(rC);k++){
   						mat[a++] = rC[k-1];
   					}
   					sigM = 1;
   					break;
   				}
   			}
   		}
   		while(rC = strtok(NULL, "::")){
   			for(int i=0;i<strlen(rC);i++){
   				if(rC[i] == ','){
   					for(int k=0;k<(i-1);k++){
   						temp[k] = rC[k];
   					}
   					if(strcmp(temp, "similar") == 0){
   						if(sigS == 1){
   							break;
   						}else{
   							simC++;
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rC);k++){
   								sim[a++] = rC[k];
   							}
   							//printf("%s\n", sim);
   							sigS = 1;
   							break;
   						}
   					}else if(strcmp(temp, "match") == 0){
   						if(sigM == 1){
   							break;
   						}else{
   							i+=2;
   							int a = 0;
   							for(int k=i;k<strlen(rC);k++){
   								mat[a++] = rC[k];
   							}
   							sigM = 1;
   							break;
   						}	
   					}
   				}
   			}
   		}
   		if(simC>0){
   			printf("The AWS received < 1 > similar words from Backend-Server < C > using UDP over port < %d >\n", SERC_PORT);
   		}else{
   			printf("The AWS received < 0 > similar words from Backend-Server < C > using UDP over port < %d >\n", SERC_PORT);
   		}
   		sigS = 0;
   		sigM = 0;
   		memset(&temp, 0, sizeof temp);
   		//memset(&sim, 0, sizeof sim);
   		memset(&dataC, 0, sizeof dataC);
   		simC = 0;
   		/*  End of receiving search result from serverC       */
   		// start to send <match> to client
   		//printf("%s\n", mat);


   	}

    if (!fork()) { // this is child process
      close(sockfd); // child doesn't need listener
      
      /*if (send(new_fd, "Hello, world!", 13, 0) == -1)
        perror("send");*/

      close(new_fd);

      exit(0);
    }
    close(new_fd); // parent doesn't need this
  }

  return 0;
}
