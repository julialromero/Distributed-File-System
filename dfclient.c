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
    config_path = argv[1];

    struct client_info info;

    display_and_handle_menu(&info);
    delete(&info);
}

void do_list(struct client_info *info, char * subfolder, char * buf){
    // create message linked list
    create_linked_list();

    // create socket and connect to servers
    parse_config_and_connect(&info, config_path);

    // parse message linked list and populate file and chunk linked lists
    struct thread_message * crawl = thread_head;
    char buf[MAXBUF];
    bzero(buf, MAXBUF);
    while(crawl != NULL){
        if(crawl->msg == NULL){
            crawl = crawl->next;
            continue;
        }

        strcpy(buf, crawl->msg);
        char * temp = strtok_r(buf, "\n", &buf);
        while(temp != NULL){
            insert_file_and_chunk_node(temp, crawl, 1);
            temp = strtok_r(buf, "\n", &buf);
        }
        bzero(buf, MAXBUF);
    }

    // iterate through file linked list and display list to user
    struct file_node * fcrawl = file_head;
    if(file_head == NULL){
        printf("Empty directory\n");
        return;
    }
    compute_if_files_are_complete();

    fcrawl = file_head;
    printf("---Listing files---\n");
    while(fcrawl != NULL){
        printf("%s", fcrawl->filename);
        if(fcrawl->is_complete != 1)
        {
            printf(" [incomplete]");
        }        
        printf("\n");
    }

    // delete message linked list
    delete_linked_list;
}

void thread(void * argument) 
{  
    /*
        - TODO
        - 
    */
    struct arg_struct *args = argument;

    int connfd = args->connfdp;
    pthread_detach(pthread_self()); 
    //free(argument);

    while(1){
        // first read socket
        // read login
        size_t n; 
        char buf[MAXBUF]; 
        bzero(buf, MAXBUF);

        n = read(connfd, buf, MAXBUF); 
        if(n < 0){
            return;
        }
        //parse_and_execute(buf, connfd);
    }


    close(connfd);
    free(args->connfdp);
    printf("Connection closed.\n\n");
    return;
}

int open_sendfd(char *ip, int port){
    int serverfd, optval=1;;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    // printf("ip: %s\n", ip);
    // printf("port: %d\n", port);

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

    // establish connection
    int sockid = connect(serverfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (sockid < 0){
        perror("Error: ");
        printf("Connection failed\n");
		return -1;
	}

    printf("Socket connected\n");

    return serverfd;
}

