#include <fcntl.h>
#ifndef _SERVERHELPER_H_
#define _SERVERHELPER_H_

struct msg_info {
    char * user;
    char * pass;
    char * cmd;
    char * file;
    char * optional_subfolder;
    char * chunk;
    char * server_dir;
};

struct arg_struct {
    int arg1;
};


char * XOR_encryption(char * str, char * pass, int strlen, int passlen);
void listFilesRecursively(char *basePath, int connfd);
void get_and_send_files_recursively(char *basePath, int connfd, char * target_file, int * done, struct msg_info *info);
void parse_header(char * request_msg, struct msg_info *info);
void * thread(void * argument);
void parse_and_execute(char * buf, int connfd); 
int authenticate_and_create_directory(int connfd, struct msg_info *info);
int check_if_valid_login(struct msg_info *info); // TODO
int open_listenfd(int port);
void delete_struct(struct msg_info *info);
int check_if_directory_exists(char * folder);
void receive_file(int connfd, struct msg_info *info); // TODO
void list_directory(int connfd, struct msg_info * info);
void send_file(char * path, int connfd, struct msg_info *info);

#endif