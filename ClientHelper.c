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

#define MAXBUF   8192 
#define LISTENQ  1024

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

        // call subroutines based on entered command
        if(strcasecmp("list", cmdline.cmd) == 0){
            // printf("Going to tolist()\n");
            do_list(cmdline);
        }

        if(strcasecmp("get", cmdline.cmd) == 0){
            do_get(cmdline);
        }

        if(strcasecmp("put", cmdline.cmd) == 0){
            do_put(cmdline);
        }
    }
}


//--------------FUNCS FOR CONNECTING TO SOCKET--------------//

struct client_info parse_config_and_connect(struct cmdlineinfo args, int to_connect){
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

    if(to_connect != 1){
        return info;
    }

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
        connect_to_server(&info, ip, portint, count, &tid, args);
    }

    // collect all threads / server responses
    for (int i = 0; i < 4; i++)
       pthread_join(tid[i], NULL);

    return info;
}

void connect_to_server(struct client_info *info, char * ip, int port, int count, pthread_t tid[4], struct cmdlineinfo cmdinfo){
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
    arg.cmd = strdup(cmdinfo.cmd);

    pthread_create(&tid[count-1], NULL, thread, (void *)&arg);

    return;
}

int open_sendfd(char *ip, int port){
    int serverfd, optval=1;;
    struct sockaddr_in serveraddr;
    
    /* Create a socket descriptor */
    if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");    // TODO: figure this out
  
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
        //perror("Error: ");
		return -1;
	}
    return serverfd;
}

void create_msg(struct client_info *info, char * dir, struct cmdlineinfo args){
    /*
        Here is the header formatting for each command --
        LIST: "user\r\npass\r\ndir\r\ncmd <subfolder>"
        GET/PUT: "user\r\npass\r\ndir\r\ncmd filepath <subfolder>"
    */
    //char * str =  "Alice\r\nSimplePassword\r\n";

   char msg[LISTENQ];
   strcpy(msg, info->user);
   strcat(msg, "\r\n");
   strcat(msg, info->pass);
   strcat(msg, "\r\n");
   strcat(msg, dir);
   strcat(msg, "\r\n");
   strcat(msg, args.cmd);

   if(strcasecmp(args.cmd, "LIST") == 0){
       info->msg = strdup(msg);
       return;
   }

   strcat(msg, " ");
   strcat(msg, args.fn);

    // printf("DEBUG: msg created: \n%s\n", msg);
   info->msg = strdup(msg);

   return;
}


//--------------FUNCS FOR STORING RECEIVED FILE CHUNK INFORMATION--------------//
struct thread_message * find_server_node(int target_server_num){
    // save the received message in the linked list node dedicated to this thread
    if(thread_head == NULL){
        printf("THREAD - Error = thread message/ head of LL is null\n");
        pthread_exit(NULL);
        return NULL;
    }
    struct thread_message * crawl = thread_head;
    while(crawl != NULL){
        if(crawl->server_num == target_server_num){
            return crawl;
            // store message in dedicated node
        }
        crawl = crawl->next;
    }
    return NULL;
}

int split_fn_and_chunk(char * fn){
    //printf("DEBUG: Received filename: %s\n", fn);
    int len = strlen(fn);
    char digit = fn[len-1];
    fn[len - 1] = '\0';
    fn[len - 2] = '\0';
    int i = digit - '0';
    return i;
}

int split_getfile_and_chunk(char * recv){
    char digit = recv[0];
    recv = recv + 1;
    int i = digit - '0';
    return i;
}

void insert_file_and_chunk_node(char * fn, char * send_msg, int is_list, int chunk_num){ //  TODO: change this to char * bc easier
    if(file_head == NULL){
        struct file_node * node = calloc(1, sizeof(struct file_node));
        node->filename = calloc(1, strlen(fn));
        
        strcpy(node->filename, fn);
        file_head = node;
        node->next = NULL;
        
        // this function will populate the chunk linked list -- there will be a node for each unique received chunk/filename chunk 
        add_chunk(node, send_msg, is_list, chunk_num);
        return;
    }

    // if file linked list exists then search for matching node
    struct file_node * crawl = file_head;
    while(crawl != NULL){
        //printf("crawl fn: %s\n", crawl->filename);
        // if file already is in linked list
        if (strcmp(fn, crawl->filename) == 0){
            add_chunk(crawl, send_msg, is_list, chunk_num);
            return;
        }
        // if this file is not in the linked list then insert it
        if(crawl->next == NULL){
            struct file_node * node = calloc(1, sizeof(struct file_node));
            node->filename = calloc(1, strlen(fn));
            strcpy(node->filename, fn);
            node->next = NULL;
            crawl->next = node;
            add_chunk(node, send_msg, is_list, chunk_num);
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
            sum += chucrawl->chunknum + 1;
            chucrawl = chucrawl->next;
        }
        
        // if all 4 parts are received, sum of chunknum is 6 (0 + 1 + 2 + 3)
        printf("Sum: %d\n", sum);
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
    
    
    // first check if this chunk has already been collected
    struct chunk_node * crawl_chunks = filenode->head;
    while(crawl_chunks != NULL){
        if(crawl_chunks->chunknum == num){
            return;
        }
        crawl_chunks = crawl_chunks->next;
    }
    //printf("DEBUG: inserting chunk node\n");  

    // create new node and add to chunk linkedlist
    struct chunk_node * new_node = calloc(1, sizeof(struct chunk_node));
    new_node->this_file = filenode;
    new_node->chunknum = num;
    printf("inserting chunk num: %d\n", new_node->chunknum);
    new_node->next = NULL;
    new_node->msg = NULL;

    if(filenode->head == NULL){
        new_node->msg = chunk_msg;
        filenode->head = new_node;
        return;
    }
    struct chunk_node * chucrawl = filenode->head;
    while(chucrawl->next != NULL){
        //printf("traversing: chunk %d\n", chucrawl->chunknum);
        chucrawl=chucrawl->next;
    }
    chucrawl->next = new_node;
  
    if(is_list == 1){
        return;
    }

    // if islist is 0 then it will update chunknode->chunk
    if(chunk_msg == NULL){
        printf("NULL!!!!\n");
    }
    new_node->msg = chunk_msg;

    return;
}

void create_linked_list(){
    int i;
    for(i = 0; i < 4; i++){
        struct thread_message * node = malloc(sizeof(struct thread_message));
        node->next = thread_head;
        thread_head = node;
        thread_head->msg = NULL;
        thread_head->connfd = -1;
        thread_head->server_num = 4-i;
        thread_head->send_chunks0 = NULL;
        thread_head->send_chunks1 = NULL;
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
         if(temp->send_chunks0 != NULL)
            free(temp->send_chunks0);
         if(temp->send_chunks1 != NULL)
            free(temp->send_chunks1);
         free(temp);
     }
     
     
     // delete other linked lists
     if(file_head == NULL){
         return;
     }
     struct file_node * fn;
     while(file_head != NULL){
         delete_chunk_list(file_head->head);
         free(file_head->filename);

         fn = file_head;
         file_head = file_head->next;
         free(fn);
     }
     thread_head = NULL;
     file_head = NULL;

    return;
}

void delete_chunk_list(struct chunk_node * head){
    struct chunk_node * temp = head;
    while(head != NULL){
        temp = head;
        //printf("deleting num%d\n", temp->chunknum);
        head = head->next;
        //free(temp);
    }
}


//--------------PUT HELPER FUNCS--------------//

// Encryption method
char * XOR_encryption(char * str, char * pass, int strleng, int passlen){
	
    char * result = (char *)calloc(1, sizeof(char) * strleng);
    int i;
	for (i = 0; i < strleng; i++) {
		result[i] = str[i] ^ pass[i % passlen]; // XOR operation between password and every character in string
	}
   
	return result;
}

// Run system call for md5. Adapted for MacOS.
int md5sum(char *file_name)
{
    char buf[MAXBUF];
    bzero(buf, MAXBUF);

    int length = strlen("md5") + strlen(file_name) + 2;
    char *sys_command = (char*)malloc((length * sizeof(char)) + 1 );

    snprintf(sys_command, length, "%s %s", "md5",file_name);

    system(sys_command );

    //printf( "attempting to popen %s \n", full_command );
    FILE *md5 = popen(sys_command, "r" );

    while( fgets(buf, sizeof(buf), md5 ) != 0 );

    pclose( md5 );

    int i, k, sum = 0;
    for(i=0; i < strlen(buf); i++){
        k = buf[i] - '0';
        sum += k;
    }    

    printf("MD5SUM: %d\n", sum);

    return sum;
}

int get_file_length(FILE * fp){
    // get size of file
    fseek(fp, 0L, SEEK_END);
    int res = ftell(fp);

    fseek(fp, 0L, SEEK_SET);
    return res;
}

int pick_delegation_scheme(int x){
     // server delegation scheme based on x
    if(x == 0){
        return 0; // { {1, 2}, {2, 3}, {3, 4}, {4, 1}};
    }
    else if(x == 1){
        return 3; //{{4, 1}, {1, 2}, {2, 3}, {3, 4}};
    }
    else if(x == 2){
        return 2; //{{3, 4}, {4, 1}, {1, 2}, {2, 3}};
    }
    else if(x == 3){
        return 1; //{{2, 3}, {3, 4}, {4, 1}, {1, 2}};
    }
    return 1;
}

void separate_file_to_chunks_and_store(int floor, FILE * fp, int x, struct client_info cinfo){
    // read each chunk into file buf
    char file[MAXBUF];
    bzero(file, MAXBUF);
    fseek(fp, 0L, SEEK_SET);
    size_t num_bytes;
    int pos = 0;

    int server_num = pick_delegation_scheme(x);

    int i;
    for(i = 0; i < 3; i++){
        bzero(file, MAXBUF);
        num_bytes = fread(file, 1, floor, fp);
        pos += (int)num_bytes;
        fseek(fp, pos, SEEK_SET);

        delegate_chunk_to_server(&server_num, file, i, num_bytes, cinfo); 
 
    }
    bzero(file, MAXBUF);
    num_bytes = fread(file, 1, floor + 6, fp);  // this will read to the end of the file since the remainder < 4
    pos += (int)num_bytes;
    fseek(fp, pos, SEEK_SET);

    delegate_chunk_to_server(&server_num, file, i, num_bytes, cinfo); 

}

// this function stores the chunk with the corresponding server node based on x, the MD5SUM
void delegate_chunk_to_server(int * server_num, char * file, int chunk_num, int filelen, struct client_info cinfo){
    // for each assigned server
    int i;
   
    int length = snprintf( NULL, 0, "%d", chunk_num);
    char* str = malloc( length + 1 );
    snprintf(str, length + 1, "%d", chunk_num);
    
    int len = strlen(str) + filelen;
    char * this_send_chunk = (char * )calloc(1, sizeof(char) * len);
    bzero(this_send_chunk, len);
    strcpy(this_send_chunk, str);
    strcat(this_send_chunk, file);

    free(str);

    // encrypt file chunk
    char * encrypted = XOR_encryption(this_send_chunk, cinfo.pass, len, strlen(cinfo.pass));

    // encrypted message is not a valid c string (with null termination) so we have to manually do some operations 
    // now copy encrypted msg over
    for(int i = 0; i < len; i++){
        this_send_chunk[i] = encrypted[i];
    }
    free(encrypted);

    for(i = 0; i < 2; i++){
        //printf("Delegating to server %d\n", *server_num);
        struct thread_message * crawl = thread_head;
        while(crawl != NULL){
            // if this node is the assigned server than add
            if(crawl->server_num - 1 == *server_num){
                
                if(crawl->send_chunks0 == NULL){
                    crawl->send_chunks0 = (char * )calloc(1, sizeof(char) * len+4); // give some extra memory for the encrypted chars
                    memcpy(crawl->send_chunks0, this_send_chunk, len);
                    crawl->chunk0_len = len;
                    
                }
                else{
                    crawl->send_chunks1 = (char * )calloc(1, sizeof(char) * len+4);
                    memcpy(crawl->send_chunks1, this_send_chunk, len);
                    crawl->chunk1_len = len;
                }
            }
            crawl = crawl->next;
        }
        if(i == 0){
            *server_num = (*server_num + 1) % 4; // bound betweeen 1 to 4
        }
    }
    //free(this_send_chunk);
}

void print_thread_linked_list(){
    printf("\n\n");
    struct thread_message * crawl = thread_head;
    while(crawl != NULL){
        printf("\n--Server %d--\n", crawl->server_num);
        // printf("connnection id: %d\n",crawl->connfd);
        if(crawl->send_chunks0 != NULL){
            printf("Chunk 1: %c\n", crawl->send_chunks0[0]);
            printf("Msg1: %s\n\n", crawl->send_chunks0);
        }
        if(crawl->send_chunks1 != NULL){
            printf("Chunk 2: %c\n", crawl->send_chunks1[0]);
            printf("Msg2: %s\n\n", crawl->send_chunks1);
        }
        crawl = crawl->next;
    }
    printf("End of LL\n");
}

void print_linked_list_get(){
    struct thread_message * crawl = thread_head;
    while(crawl != NULL){
        printf("\n--Server %d--\n", crawl->server_num);
        printf("%s\n", crawl->msg);
        crawl = crawl->next;
    }
}