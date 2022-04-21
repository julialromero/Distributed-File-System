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

struct arg_struct {
    int arg1;
};

int main(int argc, char **argv) 
{
    struct arg_struct args;
    int  *connfdp, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid; 
    int serverfd1, serverfd2, serverfd3, serverfd4;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    char * config_path = argv[1];

    struct client_info info;
    parse_config(&info, config_path);

    // ----- open socket for each server ----- //
    printf("opening socket\n");
    serverfd1 = open_sendfd(info.ip1, *info.port1);
    if(serverfd1 == -1){
        // send failure
        printf("Failed to open socket\n");
    }
    // serverfd2 = open_sendfd(info.ip2, *info.port2);
    // if(serverfd2 == -1){
    //     // send failure
    //     printf("Failed to open socket\n");
    // }
    // serverfd3 = open_sendfd(info.ip3, *info.port3);
    // if(serverfd3 == -1){
    //     // send failure
    //     printf("Failed to open socket\n");
    // }
    // serverfd4 = open_sendfd(info.ip4, *info.port4);
    // if(serverfd4 == -1){
    //     // send failure
    //     printf("Failed to open socket\n");
    // }
    info.serverfd1 = malloc(sizeof(serverfd1));
    info.serverfd2 = malloc(sizeof(serverfd2));
    info.serverfd3 = malloc(sizeof(serverfd3));
    info.serverfd4 = malloc(sizeof(serverfd4));
    *info.serverfd1 = serverfd1;
    // *info.serverfd2 = serverfd2;
    // *info.serverfd3 = serverfd3;
    // *info.serverfd4 = serverfd4;


    char buf[MAXBUF];
    while(1){
        /* get a message from the user */
        printf("Enter one of the following commands:\n");
        printf("list\nget [filename]\nput [filename]\n\n");
        bzero(buf, MAXBUF);
        fgets(buf, MAXBUF, stdin);

        // Split the message on space
        char * copy = strdup(buf);
        char * cmd, *fn, *subfolder;
        cmd = strdup(strtok_r(copy, " ", &copy));
        fn = strtok_r(copy, " ", &copy);
        if(fn != NULL){
            fn = strdup(fn);
            subfolder = strtok_r(copy, " ", &copy);
            if(subfolder != NULL){
                subfolder =  strdup(subfolder);
            }
        }

        printf("cmd: %s\n", cmd);
        if(fn != NULL){
            printf("fn: %s\n", fn);
        }

        // construct the message to send, store in buf
        create_msg(cmd, fn, subfolder, buf);

        // call subroutines based on entered command
        if(strcasecmp("list", cmd) == 0){
            do_list();
        }

        if(strcasecmp("get", cmd) == 0){}

        if(strcasecmp("put", cmd) == 0){}
    
    }

    delete(&info);
}


// void thread(void * argument) 
// {  
//     struct arg_struct *args = argument;

//     int connfd = args->arg1;
//     pthread_detach(pthread_self()); 
//     //free(argument);

//     while(1){
//         // first read socket
//         // read login
//         size_t n; 
//         char buf[MAXBUF]; 
//         bzero(buf, MAXBUF);

//         n = read(connfd, buf, MAXBUF); 
//         if(n < 0){
//             return;
//         }
//         //parse_and_execute(buf, connfd);
//     }

//     close(connfd);
//     printf("Connection closed.\n\n");
//     return;
// }

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

