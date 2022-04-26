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
#include <dirent.h>

#include "ServerHelper.h"

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXFILEBUF 60000

int mkdir(const char *pathname, mode_t mode);

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
    if(listenfd < 0){
        printf("listen unsuccessful\n");
    }
    printf("port: %d\n", port);
    while (1) {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
        args.arg1 = *connfdp;
        
        pthread_create(&tid, NULL, thread, (void *)&args);
    }
}

/* thread routine */
void * thread(void * argument) 
{  
    //printf("Connected\n");
    struct arg_struct *args = argument;

    int connfd = args->arg1;
    pthread_detach(pthread_self()); 

    int i = 0;
    while(1){
        // first read socket
        int n; 
        char buf[MAXBUF]; 
        bzero(buf, MAXBUF);

        n = read(connfd, buf, MAXBUF); 
        if(n < 0){
            
            return NULL;
        }
        if(n == 0){
            // timeout after not recieving anything
            if(i >= 20){
                printf("Socket closed\n");
                break;
            }
            i++;
            sleep(1);
            continue;
        }
        printf("Num bytes: %d\n", n);
        printf("RECEIVED: %s\n", buf);
        parse_and_execute(buf, connfd);
        return NULL;
    }

    // printf("Connection closed.\n\n");
    return NULL;
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
    
    if(strcasecmp("PUT", info.cmd) == 0){
        printf("put\n");
        receive_file(connfd, &info);

        // send confirmation
        char msg[MAXBUF];
        bzero(msg, MAXBUF);
        sprintf(msg, "File stored in server directory: %s", info.server_dir);
        //strcpy(msg, "File stored in server directory: %s", info.server_dir);
        write(connfd, msg, strlen(msg));
    }

    // if(strcasecmp("GET", info->cmd){
    //     // get all files with prefix info->file and send them back
    //     continue;
    // }

    if(strcasecmp("LIST", info.cmd) == 0){
        // send back a list all files
        char path[LISTENQ];
        strcpy(path, "./");
        strcat(path, info.server_dir);
        strcat(path, "/");
        strcat(path, info.user);
        strcat(path, "/");

        listFilesRecursively(path, connfd);
       
    }
    close(connfd);
    printf("Done sending\n");
    return;
}
