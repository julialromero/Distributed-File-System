#ifndef _CLIENTHELPER_H_
#define _CLIENTHELPER_H_

struct client_info {
    char * user;
    char * pass;
    char * msg;
};

struct arg_struct {
    int * server_num;
    int * connfdp;
    struct client_info *info;
};

struct thread_message {
    int server_num;
    char * msg;
    struct thread_message * next;
};

struct file_node {
    char * filename;
    struct file_node * next;
    struct chunk_node * head;
    int is_complete; // 1 if file is complete, otherwise 0
};

struct chunk_node {
    int chunknum;
    struct file_node * this_file;
    struct chunk * next;
    char * msg;
};

char * config_path;
struct thread_message *thread_head;
struct file_node *file_head;

void insert_file_and_chunk_node(char * fn, struct thread_message *thread_node, int is_list);
void add_chunk(struct file_node *filenode, char * chunk_msg, int is_list);  // TODO
void create_linked_list();
void delete_linked_list();
void thread(void * argument);   
void create_msg(char *cmd, char *fn, char *subfolder, char *buf);   // TODO
void delete(struct client_info *info);
int open_sendfd(char *ip, int port);
void parse_config_and_connect(struct client_info *info, char * path); // TODO: open directory and parse lines
#endif