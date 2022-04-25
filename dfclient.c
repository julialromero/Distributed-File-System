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

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */
#define MAXFILEBUF 60000



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
    parse_config_and_connect(cmdline);

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
            insert_file_and_chunk_node(temp, crawl, 1);
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

void * thread(void * argument) 
{  
    struct arg_struct args = *(struct arg_struct*) argument;
    //struct arg_struct *args = argument;
    int connfd = args.connfdp;
    //printf("\n\n\nconfdf: %d\n", connfd);
    int server_num = args.server_num;
    printf("THREAD - Connected to Server %d\n", server_num);
    char msg[MAXBUF];
    bzero(msg, MAXBUF);
    strcpy(msg, args.msg);

    //pthread_detach(pthread_self()); 

    write(connfd, msg, strlen(msg));

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

    // printf("Bytes received: %d\n\n", size);
    // printf("------Received----\n%s\n", cache_buf);

    // save the received message in a linked list node
    if(thread_head == NULL){
        printf("THREAD - Error = thread message/ head of LL is null\n");
        pthread_exit(NULL);
        return NULL;
    }
    struct thread_message * crawl = thread_head;
    while(crawl != NULL){
        //printf("THREAD - crawl server num: %d\n", crawl->server_num);
        if(crawl->server_num == server_num){
            crawl->msg = calloc(1, strlen(cache_buf));
            strcpy(crawl->msg, cache_buf);
            //printf("THREAD - Message recieved and stored.\n\n");
            break;
        }
        crawl = crawl->next;
    }
    free(cache_buf);
    close(connfd);
    //printf("THREAD - Connection closed\n");
    pthread_exit(NULL);
    return NULL;
}

int open_sendfd(char *ip, int port){
    int serverfd, optval=1;;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");    // TODO: figure this out
    //bcopy((char *)ip, (char *)&serveraddr.sin_addr.s_addr, strlen(ip));

    // construct server struct
    serveraddr.sin_family = AF_INET;  
     
    if(port > 0){
        serveraddr.sin_port = htons((unsigned short)port);
    }
    else{
        serveraddr.sin_port = htons(990);
    }
    //printf("%d\n", port);

    // establish connection
    int sockid = connect(serverfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (sockid < 0){
        //perror("Error: ");
		return -1;
	}

    // printf("Socket connected\n");

    return serverfd;
}

