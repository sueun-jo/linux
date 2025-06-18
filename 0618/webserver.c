#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* function to handle thread */
static void *clnt_connection(void *arg);
int sendData(FILE *fp, char *ct, char *filename);
void sendOk(FILE *fp);
void sendError(FILE *fp);

int main(int argc, char **argv){
    int ssock;
    pthread_t thread;
    struct sockaddr_in servaddr, cliaddr;
    unsigned int len;

    /* take the port number as argument when running the program */
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        return -1;
    }

    /* create a socket */
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if(ssock == -1){
        perror("socket()");
        return -1;
    }

    /* bind the socket with taken port number to operating system */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = (argc!=2)?htons(8080):htons(atoi(argv[1]));
    if(bind(ssock, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1){
        perror("bind()");
        return -1;
    }

    /* listen for incoming connections and set the maximum number of connections to 10 */
    if(listen(ssock, 10) == -1){
        perror("listen()");
        return -1;
    }

    while(1){
        char mesg[BUFSIZ];
        int csock;

        /* waiting for incoming connections from clients */
        len = sizeof(cliaddr);
        csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);

        /* change the network address to string */
        inet_ntop(AF_INET, &cliaddr.sin_addr, mesg, BUFSIZ);
        printf("New client connected: %s:%d\n", mesg, ntohs(cliaddr.sin_port));

        /* handle the request from client in a new thread when a new client connects */
        pthread_create(&thread, NULL, clnt_connection, (void *)&csock);
        pthread_join(thread, NULL);    // handle the continuous request from client
    }
    return 0;
}

void *clnt_connection(void *arg){
    /* transfer the argument to integer taken via thread */
    int csock = *(int *)arg;
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[BUFSIZ], type[BUFSIZ];
    char filename[BUFSIZ], *ret;

    /* transfer the file descriptor to read and write FILE stream*/
    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    /* read the request from client */
    /* read the string line by line saving it in reg_line */
    fgets(reg_line, BUFSIZ, clnt_read);

    /* print the string in reg_line */
    fputs(reg_line, stdout);

    /* extract the method and path and protocol with ' ' character as delimiter from the reg_line */
    ret = strtok(reg_line, "/ ");
    strcpy(method, (ret != NULL)?ret:"");
    if(strcmp(method, "POST") == 0){    /* if the method is POST, read the data from client */
        sendOk(clnt_write);
        goto END;
    }
    else if(strcmp(method, "GET") != 0){    /* if the method is not GET, send errd message */
        sendError(clnt_write);       goto END;
    }

    ret = strtok(NULL," ");    /* extract the filename from the path */
    strcpy(filename, (ret != NULL)?ret:"");

    if(filename[0] == '/'){    /* if the filename starts with '/', omit it */
        for(int i = 0, j = 0; i < BUFSIZ; i++, j++){
            if(filename[0] == '/') j++;
            filename[i] = filename[j];
            if(filename[i] == '\0') break;
        }
    }

    /* print the header of the response and ignore the rest of the line */
    do{
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
        char *str = strchr(reg_buf, ':');
    }while(strncmp(reg_line, "\r\n", 2));     // request header ends with \r\n

    /* send the content of the file to client using filename*/
    if(sendData(clnt_write, type, filename) == -1){
        perror("sendData()");
        exit(-1);
    }

END:
    fclose(clnt_read);  /* close the file descriptor for reading */
    fclose(clnt_write);
    
    pthread_exit(0);  /* exit the thread */

    return (void *)NULL;
}

int sendData(FILE *fp, char *ct, char *filename){

    /* response message to client for success */
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[] = "Content-Type:text/html\r\n";
    char end[] = "\r\n";            // end of the response message is always \r\n
    char buf[BUFSIZ];
    int fd, len;

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filename, O_RDWR);    // open the file in read and write mode
    do {
        len = read(fd, buf, BUFSIZ);    // read the file content to buf and send it to client
        fputs(buf, fp);
    }while(len == BUFSIZ);
    
    close(fd);      // close the file descriptor
    return 0;
}

void sendOk(FILE *fp){
    /* response message to client for success */
    char protocol[] = "HTTP/1.1 200 OK\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}

void sendError(FILE *fp){
    /* response message to client for error */
    char protocol[] = "HTTP/1.1 400 Bad Request\r\n";
    char server[] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_len[] = "Content-Length:1024\r\n";
    char cnt_type[] = "Content-Type:text/html\r\n\r\n";

    /* the content of HTML to print on the browser */
    char content1[] = "<html><head><title>Bad Connection</title></head>";
    char content2[] = "<body><font size=5>Bad Request</font></body></html>";

    printf("send error\n");
    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content1, fp);
    fputs(content2, fp);
    fflush(fp);
}
