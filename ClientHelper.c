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

#define MAXBUF   8192 

/*
    TODO for list:
    - send list command to all four ips
    - 
    - check if all 4 chunks are received
    - if not then concat [incomplete] to end of ls
*/


void create_msg(char *cmd, char *fn, char *subfolder, char *buf){
    bzero(buf, MAXBUF);
    // TODO: concat message in format:
    /*
        LIST: user\r\npass\r\ncmd <subfolder>
        GET/PUT: user\r\npass\r\ncmd filepath <subfolder>
    */
   strcpy(buf, "Alice\r\n\SimplePassword\r\n\list");
   return;
}

void parse_config_and_connect(struct client_info *info, char * path){
    // open file and parse this info
    FILE * fp;
    fp = open(path, "rb");
    char *ip, *port;
    int portint;
    
    char *buf[MAXBUF];
    bzero(buf, MAXBUF);
    int num_bytes = fread(buf, 1, MAXBUF, fp);

    // parse info from config
    char * temp = strtok_r(buf, " ", &buf);
    info->user = strtok_r(buf, "\n", &buf);
    info->user = strdup(info->user);

    temp = strtok_r(buf, " ", &buf);
    temp = strtok_r(buf, " ", &buf);
    info->pass = strtok_r(buf, "\n", &buf);
    info->pass = strdup(info->pass);

    while(1){
        temp = strtok_r(buf, " ", &buf);
        if(temp == NULL){
            break;
        }
        temp = strtok_r(buf, " ", &buf);
        ip = strtok_r(buf, ":", &buf);
        ip = strdup(ip);
        port = strtok_r(buf, "\n", &buf);
        port = strdup(port);
        portint = atoi(port);
        connect_to_server(info, ip, portint);
    }
}

void connect_to_server(struct client_info *info, char * ip, int port){
    // ----- open socket for each server ----- //
    printf("opening socket\n");
    int i, serverfd;
    int  *connfdp, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    struct arg_struct args;
    pthread_t tid; 

    serverfd = open_sendfd(ip, port);

    connfdp = malloc(sizeof(int));
    *connfdp = accept(serverfd, (struct sockaddr*)&clientaddr, &clientlen);
    args.connfdp = connfdp;
    
    pthread_create(&tid, NULL, thread, (void *)&info);


    // call next routine here

    return;
}

void delete(struct client_info *info){
    free(info->user);
    free(info->pass);
}

void display_and_handle_menu(struct client_info *info){
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

        // wait for threads to end????
    }
}

