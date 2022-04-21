#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "ClientHelper.h"

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXFILEBUF 60000



int main(int argc, char **argv) 
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    config_path = argv[1];

    struct client_info info;

    display_and_handle_menu(&info);
    delete(&info);
}

void do_list(struct client_info *info, char * fn, char * subfolder, char * buf){
            // create socket and connect to servers
        parse_config_and_connect(&info, config_path);
}

void thread(void * argument) 
{  
    struct arg_struct *args = argument;

    int connfd = args->connfdp;
    pthread_detach(pthread_self()); 
    //free(argument);

    while(1){
        // first read socket
        // read login
        size_t n; 
        char buf[MAXBUF]; 
        bzero(buf, MAXBUF);

        n = read(connfd, buf, MAXBUF); 
        if(n < 0){
            return;
        }
        //parse_and_execute(buf, connfd);
    }


    close(connfd);
    free(args->connfdp);
    printf("Connection closed.\n\n");
    return;
}

int open_sendfd(char *ip, int port){
    int serverfd, optval=1;;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // printf("ip: %s\n", ip);
    // printf("port: %d\n", port);

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");    // TODO: figure this out
    //bcopy((char *)ip, (char *)&serveraddr.sin_addr.s_addr, strlen(ip));

    // construct server struct
    serveraddr.sin_family = AF_INET;  
     
    if(port > 0){
        serveraddr.sin_port = htons((unsigned short)port);
    }
    else{
        serveraddr.sin_port = htons(990);
    }

    // establish connection
    int sockid = connect(serverfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (sockid < 0){
        perror("Error: ");
        printf("Connection failed\n");
		return -1;
	}

    printf("Socket connected\n");

    return serverfd;
}

