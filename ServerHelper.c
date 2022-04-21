#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h> 
#include <dirent.h>
#include <errno.h>

#include "ServerHelper.h"

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

    // while loop and write file to disc
    size_t n; 
    char buf[MAXBUF]; 
    bzero(buf, MAXBUF);
    n = read(connfd, buf, MAXBUF); 

    // write to disc
    return;
}

void parse_header(char * request_msg, struct msg_info *info){
    char * user = strtok_r(request_msg, "\r\n", &request_msg);
    user =strdup(user);
    
    char * pass = strtok_r(request_msg, "\r\n", &request_msg); 
    pass = strdup(pass);
    
    char * cmdline = strtok_r(request_msg, "\r\n", &request_msg);
    cmdline = strdup(cmdline);

    char * cmd = strtok_r(cmdline, " ", &cmdline);
    cmd = strdup(cmd);


    char * filepath, * optional, * chunk;
    if(strcasecmp("LIST", cmd) == 0){
        // check if optional arg
        optional = strtok_r(cmdline, " ", &cmdline);
        if(optional != NULL){
            optional = strdup(optional);
        }
        // printf("user: %s\n", user);
        // printf("pass: %s\n", pass);
        // printf("cmd: %s\n", cmd);
        // printf("optfile: %s\n\n", optional);
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
        chunk = strtok_r(cmdline, " ", &cmdline);
        chunk = strdup(chunk);
        optional = strtok_r(cmdline, " ", &cmdline);
        if(optional != NULL){
            optional = strdup(optional);
        }
       
    }
    
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

// returns 1 if successful, returns 0 if not
int get_file(FILE * fp, char * buf, int connfd, int i)
{
    printf("Sending file\n");
    // if(strcasecmp(path, "www/") == 0){
    //     bzero(path, MAXBUF);
    //     strcpy(path, "www/index.html");
    // }

    int num_bytes;
    while(1){
        bzero(buf, MAXBUF);
        num_bytes = fread(buf, 1, MAXBUF, fp);
        // printf("Num bytes sending: %d\n", num_bytes);

        // stop if end of file is reached
        if (num_bytes == 0){ 
            break;
        }
        if (i == 0){
            char * headerspace = "\r\n\r\n";
            write(connfd, headerspace, strlen(headerspace));
            i++;
        }
        // send file chunk
        write(connfd, buf, num_bytes);
    }
    return 1;
}


// "USER1\r\nPASS1\r\nPUT 1.txt chunk# optional"