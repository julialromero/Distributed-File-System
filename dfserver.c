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
#include <sys/stat.h>
#include <sys/types.h>

#include "ServerHelper.h"
#include "ClientHelper.h"

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXFILEBUF 60000

int mkdir(const char *pathname, mode_t mode);

struct arg_struct {
    int arg1;
};

// listen on socket and 
int main(int argc, char **argv) 
{
    struct arg_struct args;
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 

    if ((argc != 2) & (argc != 3)) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    

    listenfd = open_listenfd(port);
    printf("port: %d\n", port);
    while (1) {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        args.arg1 = *connfdp;
        
        pthread_create(&tid, NULL, thread, (void *)&args);
    }
}

/* thread routine */
void thread(void * argument) 
{  
    printf("Connected\n");
    struct arg_struct *args = argument;

    int connfd = args->arg1;
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
        printf("Num bytes; %d\n", n);
        printf("RECEIVED: %s\n", buf);
        parse_and_execute(buf, connfd);
    }

    close(connfd);
    printf("Connection closed.\n\n");
    return;
}

void parse_and_execute(char * buf, int connfd){
    // extract info from message
    struct msg_info info;
    parse_header(buf, &info);
 
     // check valid login
    int booll = authenticate_and_create_directory(connfd, &info);
    if(booll != 1){
        return;
    }

    // if(strcasecmp("PUT", info->cmd){
    //     receive_file(connfd, &info);
    // }

    // if(strcasecmp("GET", info->cmd){
    //     // get all files with prefix info->file and send them back
    //     continue;
    // }
    if(strcasecmp("LIST", info.cmd) == 0){
        // send back a list all files
        printf("Listing\n");

    }

    delete_struct(&info);
    return;
}

int authenticate_and_create_directory(int connfd, struct msg_info *info){
    // check if login is valid
    int booll = check_if_valid_login(info);
    if(booll != 1){
        char buf[LISTENQ];
        bzero(buf, LISTENQ);
        strcpy(buf, "Invalid Username/Password. Please try again.");  
        write(connfd, buf, strlen(buf));
        return 0;
    }

    // check if directory exists, if not then create
    int i = check_if_directory_exists(info->user);
    if (i != 1){
        mkdir(info->user, 0777);
    }

    return 1;
}

int check_if_valid_login(struct msg_info *info){
    // function to read dfs.conf and search for matching username password
    // returns 1 if valid, 0 if not
 
    return 1;
}

/* 
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure 
 */
int open_listenfd(int port) 
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;
  
    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    serveraddr.sin_port = htons((unsigned short)port); 
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;

    return listenfd;
} /* end open_listenfd */