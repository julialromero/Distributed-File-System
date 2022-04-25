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
#define LISTENQ  1024

/*
    TODO for list:
    - send list command to all four ips
    - 
    - check if all 4 chunks are received
    - if not then concat [incomplete] to end of ls
*/


void create_msg(struct client_info *info, char * temp, struct cmdlineinfo args){
    // TODO: concat message in format:
    /*
        LIST: user\r\npass\r\ncmd <subfolder>
        GET/PUT: user\r\npass\r\ncmd filepath <subfolder>
    */
   char * str =  "Alice\r\nSimplePassword\r\n";
   char msg[LISTENQ];
   strcpy(msg, str);
   strcat(msg, temp);
   strcat(msg, "\r\n");
   strcat(msg, "list");

    // printf("DEBUG: msg created: \n%s\n", msg);
   info->msg = strdup(msg);

   return;
}

void parse_config_and_connect(struct cmdlineinfo args){
    // open file and parse this info
    FILE * fp;
    char temp1[LISTENQ];
    strcpy(temp1, config_path);

    fp = fopen(temp1, "r");

    if (!fp)
        perror("fopen");

    char *ip, *port;
    int portint;
    
    char bufinit[MAXBUF];
    bzero(bufinit, MAXBUF);
    int num_bytes = fread(bufinit, 1, MAXBUF, fp);

    // parse info from config
    struct client_info info;
    char * buf = strdup(bufinit);
    char * temp = strtok_r(buf, " ", &buf);
    char * user = strtok_r(buf, "\n", &buf);
    info.user = strdup(user);

    temp = strtok_r(buf, " ", &buf);
    temp = strtok_r(buf, "\n", &buf);
    info.pass = strdup(temp);

    // construct the message to send, store in buf
    int count = 0;
    pthread_t tid[4];   // assume we know there are 4 servers
    while(1){
        temp = strtok_r(buf, " ", &buf);
        if(temp == NULL){
            break;
        }
        temp = strtok_r(buf, " ", &buf);    //server directory name
        create_msg(&info, temp, args);

        ip = strtok_r(buf, ":", &buf);
        ip = strdup(ip);

        port = strtok_r(buf, "\n", &buf);
        port = strdup(port);
        portint = atoi(port);

        count++;
        connect_to_server(&info, ip, portint, count, &tid);
    }

    // collect all threads / server responses
    for (int i = 0; i < 4; i++)
       pthread_join(tid[i], NULL);

    return;
}

void connect_to_server(struct client_info *info, char * ip, int port, int count, pthread_t tid[4]){
    // ----- open socket and spawn thread for each server ----- //
    int i, serverfd;
    int  *connfdp, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    struct arg_struct arg;

    serverfd = open_sendfd(ip, port);
    if (serverfd < 0){
        printf("Failed to connect to Server %d at Port %d\n", count, port);
        return;
    }

    arg.connfdp = serverfd;
    arg.server_num = count;
    
    char buf[MAXBUF];
    bzero(buf, MAXBUF);
    strcpy(buf, info->msg);

    arg.msg = strdup(buf);

    pthread_create(&tid[count-1], NULL, thread, (void *)&arg);

    return;
}

void display_and_handle_menu(){
    char buf[MAXBUF];
    char * copy, *cmd, *fn, *subfolder;
    while(1){
        /* get a message from the user */
        printf("\nPROGRAM: Enter one of the following commands:\n");
        printf("list\nget [filename]\nput [filename]\n\n");
        bzero(buf, MAXBUF);
        fgets(buf, MAXBUF, stdin);

        // Split the message on space
        copy = strdup(buf);
        cmd = strdup(strtok_r(copy, " ", &copy));
        fn = strtok_r(copy, " ", &copy);
        if(fn != NULL){
            fn = strdup(fn);
            subfolder = strtok_r(copy, " ", &copy);
            if(subfolder != NULL){
                subfolder =  strdup(subfolder);
                subfolder[strlen(subfolder) - 1]='\0';
            }
            else{
                fn[strlen(fn) - 1]='\0';
            }
        }
        else{
            cmd[strlen(cmd) - 1]='\0';
        }

        // populate struct with commandline input
        struct cmdlineinfo cmdline;
        cmdline.cmd = cmd;
        cmdline.fn = fn;
        cmdline.subfolder = subfolder;

        if(cmdline.fn != NULL){
            printf("fn: %s\n", cmdline.fn);
        }
        printf("cmd:--%s--\n", cmdline.cmd);   

        // call subroutines based on entered command
        if(strcasecmp("list", cmdline.cmd) == 0){
            // printf("Going to tolist()\n");
            do_list(cmdline);
        }

        if(strcasecmp("get", cmdline.cmd) == 0){}

        if(strcasecmp("put", cmdline.cmd) == 0){}

        // // free memory
        // free(cmd);
        // free(copy);
        // if(fn != NULL){
        //     free(fn);
        //     if(subfolder != NULL){
        //         free(subfolder);
        //     }
        // }
    }
}

int split_fn_and_chunk(char * fn){
    int len = strlen(fn);
    char digit = fn[len-1];
    fn[len - 1] = '\0';
    fn[len - 2] = '\0';
    int i = digit - '0';
    return i;
}

void insert_file_and_chunk_node(char * fn, struct thread_message *thread_node, int is_list){
    printf("DEBUG: Inserting file node\n");
    int chunk_num = split_fn_and_chunk(fn);
    fn = fn + 1*sizeof(char);
    
    
    if(file_head == NULL){
        struct file_node * node = calloc(1, sizeof(struct file_node));
        node->filename = calloc(1, strlen(fn));
        
        strcpy(node->filename, fn);
        file_head = node;
        node->next = NULL;
        
        // this function will populate the chunk linked list -- there will be a node for each unique received chunk/filename chunk 
        add_chunk(node, thread_node->msg, is_list, chunk_num);
        return;
    }

    // if file linked list exists then search for matching node
    struct file_node * crawl = file_head;
    while(crawl != NULL){
        //printf("crawl fn: %s\n", crawl->filename);
        // if file already is in linked list
        if (strcmp(fn, crawl->filename) == 0){
            add_chunk(crawl, thread_node->msg, is_list, chunk_num);
            return;
        }
        // if this file is not in the linked list then insert it
        if(crawl->next == NULL){
            struct file_node * node = calloc(1, sizeof(struct file_node));
            node->filename = calloc(1, strlen(fn));
            strcpy(node->filename, fn);
            node->next = NULL;
            crawl->next = node;
            add_chunk(node, thread_node->msg, is_list, chunk_num);
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
            //printf("chunk num: %d\n", chucrawl->chunknum);
            sum += chucrawl->chunknum;
            chucrawl = chucrawl->next;
        }
        //printf("sum: %d", sum);
        // if all 4 parts are received, sum of chunknum is 10
        if(sum == 10){
            crawl->is_complete = 1;
        }
        else{
            crawl->is_complete = 0;
        }
        crawl = crawl->next;
    }

   return;
}

void add_chunk(struct file_node *filenode, char * chunk_msg, int is_list, int num){
    // this function will create a chunk node and add it to the chunk linked list
    // if is-list is 1 then it will just update chunknode->chunkname and chunknode->filename  
    printf("DEBUG: inserting chunk node\n");  
   

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
        node->server_num = 4-i;
        thread_head = node;
        thread_head->msg = NULL;
    }
    return;
}

void delete(){
     if(thread_head == NULL){
         return;
     }
     struct thread_message * temp;
     while(thread_head != NULL){
         temp = thread_head;
         thread_head = thread_head->next;
         free(temp->msg);
         free(temp);
     }
     
     // delete other linked lists
     if(file_head == NULL){
         return;
     }
     struct file_node * fn;
     while(file_head != NULL){
         free(file_head->head);
         fn = file_head;
         file_head = file_head->next;
         free(fn);
     }
     thread_head = NULL;
     file_head = NULL;


    return;
}