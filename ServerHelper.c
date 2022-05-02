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

#define LISTENQ  1024
#define MAXBUF   8192 

void list_directory(int connfd, struct msg_info * info){
    DIR *d;
    struct dirent *dir;
    d = opendir(info->user);

    char buf[MAXBUF];
    bzero(buf, MAXBUF);
    if (d) {
        strcpy(buf, "Listing directory:\n"); 
        int buffer_size_free;
        while ((dir = readdir(d)) != NULL) {
            buffer_size_free = MAXBUF - strlen(buf);
            int namelength = strlen(dir->d_name);
        
            // check if buffer has capacity for next filename
            if(buffer_size_free <= namelength + 1){
                write(connfd, buf, strlen(buf));
                bzero(buf, MAXBUF);
            }
            strcat(buf, "\n");
            strcat(buf, dir->d_name);
        }
    closedir(d);
    }
}

void receive_file(int connfd, struct msg_info *info){
    // rename file piece
    //printf("\n\n--receviing file---\n");

    // while loop and write file to disc
    size_t n; 
    char buf[MAXBUF]; 
    bzero(buf, MAXBUF);
    n = read(connfd, buf, MAXBUF); 

    //printf("file chunk received: %s\n", buf);
    char chunk_num = buf[0];
    //printf("Received chunknum: %c\n", buf[0]);
    char * adjbuf = buf + 1;

    // write to disc
    FILE * fp;
    char path[LISTENQ];
    bzero(path, LISTENQ);
    strcpy(path, info->server_dir);
    strcat(path, "/");
    strcat(path, info->user);
    strcat(path, "/");
    strcat(path, info->file);
    strcat(path, ".");
    strncat(path, &chunk_num, sizeof(chunk_num));
    fp = fopen(path, "wb");
    fwrite(adjbuf, 1, strlen(adjbuf), fp);

    fclose(fp);

    // send confirmation
    char msg[MAXBUF];
    bzero(msg, MAXBUF);
    sprintf(msg, "File stored in server directory: %s", info->server_dir);
    write(connfd, msg, strlen(msg));

    return;
}

void parse_header(char * request_msg, struct msg_info *info){
    /*
        Here is the header formatting for each command --
        LIST: "user\r\npass\r\ndir\r\ncmd <subfolder>"
        GET/PUT: "user\r\npass\r\ndir\r\ncmd filepath <subfolder>"
    */
    //printf("DEBUG: Received header: %s\n", request_msg);
    char * user = strtok_r(request_msg, "\r\n", &request_msg);
    user =strdup(user);
    
    char * pass = strtok_r(request_msg, "\r\n", &request_msg); 
    pass = strdup(pass);

    char * dir = strtok_r(request_msg, "\r\n", &request_msg); 
    dir = strdup(dir);

    char * cmdline = strtok_r(request_msg, "\r\n", &request_msg);
    cmdline = strdup(cmdline);

    char * cmd = strtok_r(cmdline, " ", &cmdline);
    cmd = strdup(cmd);

    // now the split is conditional on the command just read
    char * filepath, * optional, * chunk;
    if(strcasecmp("LIST", cmd) == 0){
        // check if optional arg
        optional = strtok_r(cmdline, " ", &cmdline);
        if(optional != NULL){
            optional = strdup(optional);
        }
    }
    else if (strcasecmp("GET", cmd) == 0){
        filepath = strtok_r(cmdline, " ", &cmdline);
        filepath = strdup(filepath);
        optional = strtok_r(cmdline, " ", &cmdline);
        if(optional != NULL){
            optional = strdup(optional);
        }
        // printf("user: %s\n", user);
        // printf("pass: %s\n", pass);
        // printf("cmd: %s\n", cmd);
        // printf("file: %s\n", filepath);
        // printf("optfile: %s\n\n", optional);
    }
    else{
        filepath = strtok_r(cmdline, " ", &cmdline);
        filepath = strdup(filepath);

        
        // chunk = strtok_r(cmdline, " ", &cmdline);
        // chunk = strdup(chunk);
        optional = strtok_r(cmdline, " ", &cmdline);
        if(optional != NULL){
            optional = strdup(optional);
        }
       
    }
    
    info->server_dir = dir;
    info->user = user;
    info->pass = pass;
    info->cmd = cmd;
    info->file = filepath;
    info->optional_subfolder = optional;
    info->chunk = chunk;
    return;
}

void delete_struct(struct msg_info *info){
    free(info->cmd);
    free(info->file);
    free(info->pass);
    free(info->optional_subfolder);
    free(info->user);
    free(info->chunk);
    free(info);
}

int check_if_directory_exists(char * folder)
{
    DIR* dir = opendir(folder);
    if (dir)
    {
        closedir(dir);
        return 1;
    }

    return 0;
}

int checkIfFileExists(char * filename)
{
    
    FILE *file;
    if ((file = fopen(filename, "rb")))
    {
        fclose(file);
        return 1;
    }

    return 0;
}

void listFilesRecursively(char *basePath, int connfd)
{
    char path[1000];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char buf[1002];

    // Unable to open directory stream
    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            bzero(buf, 1002);
            strcpy(buf, dp->d_name);
            strcat(buf, "\n");

            printf("Filename: %s\n", buf);

            write(connfd, buf, strlen(buf));

            // Construct new path from our base path
            strcpy(path, basePath);
            strcat(path, "/");
            strcat(path, dp->d_name);

            listFilesRecursively(path, connfd);
        }
    }

    closedir(dir);
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
    char path[LISTENQ];
    strcpy(path, info->server_dir);
    strcat(path, "/");
    strcat(path, info->user);
    int i = check_if_directory_exists(path);
    if (i != 1){
        printf("making dir: %s\n", path);
        mkdir(path, 0777);
    }

    return 1;
}

// TODO
int check_if_valid_login(struct msg_info *info){
    // function to read dfs.conf and search for matching username password
    // returns 1 if valid, 0 if not

 
    return 1;
}

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
} 

// -----------GET---------------- //
void get_and_send_files_recursively(char *basePath, int connfd, char * target_file, int * done)
{
    if(*done == 2)
        return;
    char path[1000];
    char full_fn[500];
    struct dirent *dp;
    DIR *dir = opendir(basePath);
    char buf[1002];

    // Unable to open directory stream
    if (!dir)
        return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            bzero(buf, 1002);
            strcpy(buf, dp->d_name);
            
            strncpy(full_fn,  buf, strlen(buf) - 2);
            //printf("Filename: %s\n", buf);
            strcat(buf, "\n");
            //write(connfd, buf, strlen(buf));

            // Construct new path from our base path
            strcpy(path, basePath);
            //strcat(path, "/");
            strcat(path, dp->d_name);

            if(strcasecmp(full_fn, target_file) == 0){
                printf("File found: %s\n", path);
                send_file(path, connfd);
                write(connfd, "\r\n", strlen("\r\n"));

                // int n = read(connfd, buf, MAXBUF); 
                // bzero(buf);
                *done = *done + 1;
                printf("DONE: %d\n", *done);
                if(*done == 2)
                    return;
                write(connfd, "\r\n", strlen("\r\n"));
            }

            get_and_send_files_recursively(path, connfd, target_file, done);
            // if(*done == 1){
            //     return;
            // }
        }
    }

    closedir(dir);
}

void send_file(char * path, int connfd)
{
    char buf[MAXBUF-5];   // to store the file chunk
    char send_buf[MAXBUF];    // first character is file chunk number
    FILE * fp = fopen(path, "rb");
    if(fp == NULL){
        printf("Opening file is unsuccessful.\n");
        write(connfd, "File open unsuccessful", strlen("File open unsuccessful"));
        return;
    }

    //printf("--------Sending file---------\n");
    int num_bytes;

    bzero(send_buf, MAXBUF);
    
    char chunk_num = path[strlen(path) - 1];
    char convert[2];
    convert[0] = chunk_num;
    convert[1] = '\0';


    memcpy(send_buf, convert, sizeof(convert));
    bzero(buf, MAXBUF-5);
    num_bytes = fread(buf, 1, MAXBUF-5, fp);
    strcat(send_buf, buf);
    int n = write(connfd, send_buf, strlen(send_buf));
    printf("%s", send_buf);
    while(num_bytes != 0){
        // for first send, append chunk number to front of buf
        bzero(send_buf, MAXBUF);
        num_bytes = fread(send_buf, 1, MAXBUF, fp);
        

        // stop if end of file is reached
        if (num_bytes == 0){ 
            break;
        }
        
        // send file chunk
        write(connfd, send_buf, num_bytes);
        //printf("%s", send_buf);
        n  = n + num_bytes;
    }
    printf("Sent bytes: %d\n", n);
    return;
}