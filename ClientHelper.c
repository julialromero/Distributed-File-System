#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h> 
#include <dirent.h>
#include <errno.h>

#include "ClientHelper.h"

#define MAXBUF   8192 

void do_list(){

}

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

void parse_config(struct client_info *info, char * path){
    // TODO: open file and parse this info
    //FILE * fp;

    char * user = "Alice";
    char * pass = "SimplePassword";
    char * ip = "127.0.0.1";
    int port1 = atoi("10001");
    int port2 = atoi("10002");
    int port3 = atoi("10003");
    int port4 = atoi("10004");

    // allocate memory
    info->user = malloc(strlen(user)); 
    info->pass = malloc(strlen(pass));

    info->ip1 = malloc(strlen(ip));
    info->port1 = malloc(sizeof(port1));

    info->ip2 = malloc(strlen(ip));
    info->port2 = malloc(sizeof(port2));

    info->ip3 = malloc(strlen(ip));
    info->port3 = malloc(sizeof(port3));

    info->ip4 = malloc(strlen(ip));
    info->port4 = malloc(sizeof(port4));

    // assign values
    strcpy(info->user,user);
    strcpy(info->pass, pass);

    strcpy(info->ip1, ip);
    *info->port1 = port1;

    strcpy(info->ip2, ip);
    *info->port2 = port2;

    strcpy(info->ip3, ip);
    *info->port3 = port3;

    strcpy(info->ip4, ip);
    *info->port4 = port4;

    return;
}

void delete(struct client_info *info){
    free(info->user);
    free(info->pass);
    free(info->ip1);
    free(info->ip2);
    free(info->ip3);
    free(info->ip4);
    free(info->port1);
    free(info->port2);
    free(info->port3);
    free(info->port4);
    free(info->serverfd1);
    free(info->serverfd2);
    free(info->serverfd3);
    free(info->serverfd4);
}