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
    char * cmd;
    //struct client_info info;
};

struct thread_message {
    int server_num;
    int connfd;
    char * msg;
    char * send_chunks0;
    char * send_chunks1;
    int chunk0_len;
    int chunk1_len;
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

char * XOR_encryption(char * str, char * pass, int strlen, int passlen);
int split_getfile_and_chunk(char * recv);
void insert_file_and_chunk_node(char * fn, char * send_msg, int is_list, int chunk_num);
struct thread_message * find_server_node(int target_server_num);
void delete_chunk_list(struct chunk_node * head);
void print_thread_linked_list();
void print_linked_list_get();
void separate_file_to_chunks_and_store(int floor, FILE * fp, int x, struct client_info cinfo);
int pick_delegation_scheme(int x);
void delegate_chunk_to_server(int * server_num, char * file, int chunk_num, int filelen, struct client_info cinfo);
int get_file_length(FILE * fp);
int md5sum(char *file_name);
void compute_if_files_are_complete();
void connect_to_server(struct client_info *info, char * ip, int port, int count, pthread_t tid[4], struct cmdlineinfo cmdinfo);
int split_fn_and_chunk(char * fn);

void do_put(struct cmdlineinfo cmdline);
void do_list(struct cmdlineinfo cmdline);
void do_get(struct cmdlineinfo cmdline);

void display_and_handle_menu();
//void insert_file_and_chunk_node(char * fn, struct thread_message *thread_node, int is_list);
void add_chunk(struct file_node *filenode, char * chunk_msg, int is_list, int num);  // TODO
void create_linked_list();
void delete();
void * thread(void * argument);   
void create_msg(struct client_info *info, char * temp, struct cmdlineinfo args);   // TODO
int open_sendfd(char *ip, int port);
struct client_info parse_config_and_connect(struct cmdlineinfo args, int to_connect);
#endif