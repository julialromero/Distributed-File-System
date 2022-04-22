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

    int count = 0;
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
        count++;
        connect_to_server(info, ip, portint, count);
    }
}

void connect_to_server(struct client_info *info, char * ip, int port, int count){
    // ----- open socket and spawn thread for each server ----- //
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
    args.server_num = count;
    args.info = info;
    
    pthread_create(&tid, NULL, thread, (void *)&args);

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
            do_list(buf);
        }

        if(strcasecmp("get", cmd) == 0){}

        if(strcasecmp("put", cmd) == 0){}

        // wait for threads to end????
    }
}

void insert_file_and_chunk_node(char * fn, struct thread_message *thread_node, int is_list){
    if(file_head == NULL){
        struct file_node * node = calloc(1, sizeof(struct file_node));
        node->filename = calloc(1, strlen(fn));
        // TODO: make sure to remove chunk number from file number!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        strcpy(node->filename, fn);
        file_head = node;
        node->next = NULL;
        
        // this function will populate the chunk linked list -- there will be a node for each unique received chunk/filename chunk 
        add_chunk(node, thread_node->msg, is_list);
        return;
    }

    // if file linked list exists then search for matching node
    struct file_node * crawl = file_head;
    while(crawl != NULL){
        // if file already is in linked list
        if (strcmp(fn, crawl->filename) == 0){
            add_chunk(crawl, thread_node->msg, is_list);
            return;
        }
        // if this file is not in the linked list then insert it
        if(crawl->next == NULL){
            struct file_node * node = calloc(1, sizeof(struct file_node));
            node->filename = calloc(1, strlen(fn));
            strcpy(node->filename, fn);
            node->next = NULL;
            crawl->next = node;
            return;
        }
        crawl = crawl->next;
    }
    return;
}

// for each file, tally up the number of unique chunks and mark if file is complete
void compute_if_files_are_complete(){
    struct file_node * crawl = file_head;
    while(crawl != NULL){
        struct chunk_node * chucrawl = crawl->head;
        int sum = 0;
        while(chucrawl != NULL){
            sum += chucrawl->chunknum;
            chucrawl = chucrawl->next;
        }
        // if all 4 parts are received, sum of chunknum is 10
        if(sum == 10){
            crawl->is_complete = 1;
        }
        else{
            crawl->is_complete = 0;
        }
    }
   return;
}

void add_chunk(struct file_node *filenode, char * chunk_msg, int is_list){
    // this function will create a chunk node and add it to the chunk linked list
    // if is-list is 1 then it will just update chunknode->chunkname and chunknode->filename    
    char cpy_fn[] = *filenode->filename;
    char * temp = strtok_r(cpy_fn, "/.", &cpy_fn);
    while(temp != NULL){
        // first parse msg
        temp = strtok_r(cpy_fn, ".", &cpy_fn);
        temp = strtok_r(cpy_fn, ".", &cpy_fn);
        temp = strtok_r(cpy_fn, ".", &cpy_fn);
        if(temp == NULL){
            printf("parsed past period - line 218 clienthelper\n");
        }
        printf("Extracted chunk number: %s\n", temp);
    }
    int num = atoi(temp);

    // create new node and add to chunk linkedlist
    struct chunk_node * new_node = calloc(1, sizeof(struct chunk_node));
    new_node->this_file = filenode;
    new_node->chunknum = num;
    new_node->next = NULL;
    new_node->msg = NULL;

    if(filenode->head == NULL){
        filenode->head = new_node;
        return;
    }
    struct chunk_node * chucrawl = filenode->head;
    while(chucrawl->next != NULL){
        chucrawl=chucrawl->next;
    }
    chucrawl->next = new_node;
  
    if(is_list == 1){
        return;
    }

    // if islist is 0 then it will update chunknode->chunk
    new_node->msg = chunk_msg;

    return;
}

void create_linked_list(){
    int i;
    for(i = 0; i < 4; i++){
        struct thread_message * node = malloc(sizeof(struct thread_message));
        node->next = thread_head;
        node->server_num = 3-i;
        thread_head = node;
    }
    return;
}

void delete_linked_list(){
     if(thread_head == NULL){
         return;
     }
     struct thread_message * temp;
     while(thread_head != NULL){
         temp = thread_head;
         thread_head = thread_head->next;
         free(temp);
     }
    return;
}