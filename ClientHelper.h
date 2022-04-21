#include <fcntl.h>
#ifndef _CLIENTHELPER_H_
#define _CLIENTHELPER_H_

struct client_info {
    char * user;
    char * pass;
    char * msg;
};

struct arg_struct {
    int * connfdp;
    struct client_info *info;
};

struct thread_message {
    char * msg;
    struct thread_message * next;
};
char * config_path;
struct thread_message thread_head;

void thread(void * argument);
void create_msg(char *cmd, char *fn, char *subfolder, char *buf);
void delete(struct client_info *info);
int open_sendfd(char *ip, int port);
void parse_config_and_connect(struct client_info *info, char * path); // TODO: open directory and parse lines
#endif