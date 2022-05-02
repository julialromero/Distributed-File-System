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
#include <ctype.h>

#include "ClientHelper.h"

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXFILEBUF 60000
#define MD5_LEN 32


int main(int argc, char **argv) 
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    bzero(config_path, LISTENQ);
    strcpy(config_path, argv[1]);
    //printf("Config file: %s\n", config_path);

    display_and_handle_menu();
}

void do_list(struct cmdlineinfo cmdline){
    // create message linked list
    create_linked_list();

    // create socket and connect to servers
    parse_config_and_connect(cmdline, 1);

    // parse message linked list and populate file and chunk linked lists
    struct thread_message * crawl = thread_head;
    char * buf;
    if(crawl == NULL){
        printf("Failed to create thread ll?\n");
    }
    while(crawl != NULL){
        if(crawl->msg == NULL){
            crawl = crawl->next;
            continue;
        }
        //printf("Message found in LL: %d\n", crawl->server_num);

        buf = strdup(crawl->msg);
        char * temp = strtok_r(buf, "\n", &buf);
        while(temp != NULL){
            int chunk_num = split_fn_and_chunk(temp);
            insert_file_and_chunk_node(temp, crawl->msg, 1, chunk_num);
            temp = strtok_r(buf, "\n", &buf);
        }
        crawl = crawl->next;
    }

    compute_if_files_are_complete();

    // iterate through file linked list and display list to user
    struct file_node * fcrawl = file_head;
    if(file_head == NULL){
        printf("PROGRAM: Empty directory\n");
        return;
    }

    fcrawl = file_head;
    printf("\n---Listing files---\n");
    while(fcrawl != NULL){
        printf("%s", fcrawl->filename);
        if(fcrawl->is_complete != 1)
        {
            printf(" [incomplete]");
        }        
        printf("\n");
        fcrawl = fcrawl->next;
    }

    // delete message linked list
    delete();
}

void do_put(struct cmdlineinfo cmdline){
    // create message linked list
    create_linked_list();

    int sum = md5sum(cmdline.fn);
    if (sum < 0) {
        puts("Error occurred!");
    } 

    int x = sum % 4;
    printf("x value is %d\n", x);

    // calculate length of the 4 chunks
    FILE * fp = fopen(cmdline.fn, "rb");
    int len = get_file_length(fp);
    int floor = (int)len / 4;

    // separate file into chunks and store in linkedlist node corresponding to destination server
    struct client_info cinfo = parse_config_and_connect(cmdline, 0);
    separate_file_to_chunks_and_store(floor, fp, x, cinfo);

    //thread_linked_list();
    
    // finally, connect to servers
    parse_config_and_connect(cmdline, 1);

    // iterate thru connections
    //print_thread_linked_list();
    struct thread_message *crawl = thread_head;
    size_t n; 
    int connfd;
    char buf[MAXBUF]; 
    bzero(buf, MAXBUF);
    while(crawl != NULL){
        connfd = crawl->connfd;
        if(connfd < 0){     // if we did not connect to this server 
            crawl = crawl->next;
            continue;
        }
       
        n = write(connfd, crawl->send_chunks0, crawl->chunk0_len);
        printf("Send bytes: %d\n", n);

        n = read(crawl->connfd, buf, MAXBUF); 
       
       
        n = write(connfd, crawl->send_chunks1, crawl->chunk1_len);
        printf("Send bytes: %d\n", n);
        
        n = read(crawl->connfd, buf, MAXBUF); 

        crawl = crawl->next;
    }
    
    // delete message linked list
    delete();
}

void do_get(struct cmdlineinfo cmdline){
    // create message linked list
    create_linked_list();

    // create socket and connect to servers
    struct client_info cinfo = parse_config_and_connect(cmdline, 1);

    // parse message linked list and populate file and chunk linked lists
    print_linked_list_get();
    printf("-----------------\n");
    struct thread_message * crawl = thread_head;
    char * buf;
    if(crawl == NULL){
        printf("Failed to create thread ll?\n");
    }
    while(crawl != NULL){
        if(crawl->msg == NULL){
            crawl = crawl->next;
            continue;
        }
   
        buf = strdup(crawl->msg);
        char * temp1 = strtok_r(buf, "\r\n", &buf);
        temp1 = strdup(temp1);
        int chunk1 = split_getfile_and_chunk(temp1);
        temp1 = temp1+1;


        char * temp2 = strtok_r(buf, "\r\n", &buf);
        if(temp2 != NULL){
            temp2 = strdup(temp2);
        }
        int chunk2 = split_getfile_and_chunk(temp2);
        temp2 = temp2+1;


        insert_file_and_chunk_node(cmdline.fn, temp1, 0, chunk1);
        insert_file_and_chunk_node(cmdline.fn, temp2, 0, chunk2);
        crawl = crawl->next;
    }
    printf("-----------------\n");
    compute_if_files_are_complete();

    struct file_node * fcrawl = file_head;
    if(file_head == NULL){
        printf("PROGRAM: File not found\n");
        return;
    }

    if(file_head->is_complete != 1){
        printf("Printing file --\n%s [incomplete]\n", cmdline.fn);
        return;
    }
    
    // reconstruct file and save. Order  0, 1, 2, 3
    char *save_buf;   
    file_head->filename = cmdline.fn;
    struct chunk_node * ptr = file_head->head;
    int i = 0;
    int iter = 0;
    char * temp;
    int n;
    while(1){
        ptr = file_head->head;
        while(ptr != NULL){
            
            if(ptr->chunknum == i){
                FILE * fp;
                fp = fopen(file_head->filename, "ab");
                if(fp == NULL){
                    printf("Failed to open file\n");
                }
                
                if(ptr->msg == NULL){
                    printf("Msg is null?\n");
                    ptr = ptr->next;
                }
                n = strlen(ptr->msg);
                fwrite(ptr->msg, 1, n, fp);
                fclose(fp); 
                i++;
            
                // if we collected all 4 chunks then exit
                iter++;
                if(iter == 4){
                    break;;
                }
            }
            ptr = ptr->next;
        }
        if(iter == 4){
            break;
        }

    }
    // delete message linked list
    //printf("Deleting\n");
    delete();

}

void * thread(void * argument) 
{  
    /* 
        Copy over arguments
    */
    struct arg_struct args = *(struct arg_struct*) argument;
    int connfd = args.connfdp;
    int server_num = args.server_num;
    //char * cmd = args.cmd;
    
    char msg[MAXBUF];
    bzero(msg, MAXBUF);
    strcpy(msg, args.msg);

    char cmd[MAXBUF];
    bzero(cmd, MAXBUF);
    strcpy(cmd, args.cmd);

    //printf("THREAD - Connected to Server %d\n", server_num);

    write(connfd, msg, strlen(msg));

    // TODO clean this up
    if(strcasecmp("PUT", cmd) == 0){
        // save the received message in the linked list node dedicated to this thread
        if(thread_head == NULL){
            printf("THREAD - Error = thread message/ head of LL is null\n");
            pthread_exit(NULL);
            return NULL;
        }
        struct thread_message * crawl = thread_head;
        while(crawl != NULL){
            if(crawl->server_num == server_num){
                // store connection id in dedicated node
                crawl->connfd = connfd;
                break;
            }
            crawl = crawl->next;
        }
        pthread_exit(NULL);
        return NULL;
    }
    
    // receive msg
    char buf[MAXBUF]; 
    bzero(buf, MAXBUF);

    size_t n;
    char *cache_buf = calloc(MAXFILEBUF,1);
    int size = 0;
    while((n = read(connfd, buf, MAXBUF)) > 0){
        // make sure allocated memory is large enough
        if((cache_buf = realloc(cache_buf, size + n * sizeof(char))) == NULL){
            pthread_exit(NULL);
            return NULL;
        }
        memcpy(cache_buf + size, buf, n);
        size+=n;   
    }

    if(size == 0){
        free(cache_buf);
        pthread_exit(NULL);
        return NULL;
    }

    //printf("Received:--\n%s\n", cache_buf);

    printf("\n\nBytes received: %d\n", size);


    struct thread_message * target_server_node = find_server_node(server_num);
    if(target_server_node == NULL){
        printf("THREAD - Error = thread message/ head of LL is null\n");
        pthread_exit(NULL);
        return NULL;
    }
    target_server_node->msg = calloc(1, strlen(cache_buf));
    strcpy(target_server_node->msg, cache_buf);
    target_server_node->connfd = connfd;

    
    free(cache_buf);
    //printf("THREAD - Connection closed\n");
    pthread_exit(NULL);
    return NULL;
}
