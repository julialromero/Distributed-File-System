#include <fcntl.h>
#ifndef _CLIENTHELPER_H_
#define _CLIENTHELPER_H_

struct client_info {
    int * serverfd1, *serverfd2, *serverfd3, *serverfd4;

    char * user;
    char * pass;

    char * ip1;
    int * port1;

    char * ip2;
    int * port2;

    char * ip3;
    int * port3;

    char * ip4;
    int * port4;
};

void create_msg(char *cmd, char *fn, char *subfolder, char *buf);
void delete(struct client_info *info);
int open_sendfd(char *ip, int port);
void parse_config(struct client_info *info, char * path); // TODO: open directory and parse lines
#endif