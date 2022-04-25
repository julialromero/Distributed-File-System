#ifndef _CLIENTHELPER_H_
#define _CLIENTHELPER_H_

#define LISTENQ  1024
#define MAXBUF   8192 
struct cmdlineinfo {
    char * cmd;
    char * fn;
    char * subfolder;
};

struct client_info {
    char * user;
    char * pass;
    char * msg;
};

struct arg_struct {
    int server_num;
    int connfdp;
    char * msg;
    //struct client_info info;
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
    struct chunk_node * next;
    char * msg;
};

char config_path[LISTENQ];
struct thread_message *thread_head;
struct file_node *file_head;

void compute_if_files_are_complete();
void connect_to_server(struct client_info *info, char * ip, int port, int count, pthread_t tid[4]);
void do_list(struct cmdlineinfo cmdline);
void display_and_handle_menu();
void insert_file_and_chunk_node(char * fn, struct thread_message *thread_node, int is_list);
void add_chunk(struct file_node *filenode, char * chunk_msg, int is_list, int num);  // TODO
void create_linked_list();
void delete();
void * thread(void * argument);   
void create_msg(struct client_info *info, char * temp, struct cmdlineinfo args);   // TODO
int open_sendfd(char *ip, int port);
void parse_config_and_connect();
#endif