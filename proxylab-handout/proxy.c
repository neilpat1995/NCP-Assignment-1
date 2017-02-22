/*
 * proxy.c - A Simple Sequential Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 2
 * Student Name: Neil M. Patel
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

#define PROGRAM_NAME "proxy"
#define LOGFILE_NAME "proxy.log"

/*
 * Function prototypes
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);


void err_exit() {
    perror(PROGRAM_NAME);
    exit(2);
}

void process_request(int cfd, char* proxy_port, struct sockaddr_in cliaddr) {
    printf("Now starting process_request\n");
    
    //read request
    char reqbuf[10000];
    int reqlen = 0;
    if ( (reqlen = read(cfd, reqbuf, sizeof(reqbuf))) == -1 ) {
        printf("%d\n", __LINE__);
        err_exit();
    }

    printf("Now printing read request...\n");
    printf("%s\n", reqbuf);

    //Parse URI and IP address from request (second token) and socket address structure
    char* request_uri;
    strtok(reqbuf, " ");
    request_uri = strtok(NULL, " ");

    char* ip_addr = (char*) malloc(INET_ADDRSTRLEN);
    bzero(ip_addr, INET_ADDRSTRLEN);
    if ((ip_addr = inet_ntop(AF_INET, &cliaddr.sin_addr, ip_addr, INET_ADDRSTRLEN)) == NULL) {
        err_exit();
    }

    printf("Request URI is: %s\n", request_uri);
    printf("IP address is: %s\n", ip_addr);

    free(ip_addr);


    //Read log entry into buffer
    //Assumes entries are at most 1000 bytes (characters) long
    char entry_buf[1000];
    bzero(entry_buf, sizeof(entry_buf));
    format_log_entry(entry_buf, &cliaddr, request_uri, 0);
    printf("Log entry is: %s\n", entry_buf);

    //Write buffer to "proxy.log" file
    FILE *fp;
    if ((fp = fopen(LOGFILE_NAME, "a")) == NULL) {
        err_exit();
    }

    int res = fprintf(fp, "%s\n", entry_buf);

    printf("Num bytes read: %d\n", res);

    fclose(fp);

    /*
    //Write back to the socket for view in browser
    if (write(cfd, reqbuf, reqlen) < 0) {
        err_exit();
    }
    */

    /*// Format: GET <pathname> HTTP/1.0
    if ( strncmp(reqbuf, "GET", 3) != 0 ) { 
        fprintf(stderr, "Not a GET request\n");
        exit(2);
    }
    char *s = reqbuf;
    strsep(&s, " ");    //gets HTTP method only
    //char *filename = strsep(&s, " "); //gets the filename

    //write request
    char *filename = "index.html";
    
    //open file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        err_exit();

    //read contents
    char filebuf[1000];
    int flen = 0;*/
    
    /*
    char buf[] = "HTTP/1.1 200 OK\r\nContent type: text/html\r\n\r\n";
    if (write(cfd, buf, sizeof(buf)) < 0) //write header
        err_exit();
    */

    /*while ( fread(filebuf, 1, sizeof(filebuf), fp) == sizeof(filebuf) ) {
        if ( write(cfd, filebuf, flen) < 0)
            printf("%d\n", __LINE__);
            err_exit();
    }
    if ( write(cfd, filebuf, flen) < 0 )
        printf("%d\n", __LINE__);
        err_exit();
    
    if ( feof(fp) != 0 )
        printf("%d\n", __LINE__);
        err_exit();

    if ( ferror(fp) != 0 )
        printf("%d\n", __LINE__);
        err_exit();

    //close file
    if ( fclose(fp) != 0 )
        printf("%d\n", __LINE__);
        err_exit();
    printf("Now ending process_request\n");     
    */
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{

    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }

    //socket
    int s = 0;
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) //i.e. s = -1
        err_exit(argv[0]);
    
    //bind
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr)); //0 out all other fields in the struct
    servaddr.sin_family = AF_INET;  //not placed into packet- no need to check order
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // long (4 B) to network byte order
    servaddr.sin_port = htons(atoi(argv[1])); //to convert short (2 bytes) to network byte order

    if ((bind(s, (struct sockaddr *) &servaddr, sizeof(servaddr))) < 0) //nonzero (i.e. -1) means error
        err_exit(argv[0]);

    //listen
    if ((listen(s, 5)) < 0) //2nd param specifies max number of pending connections allowed
        err_exit(argv[0]);
    
    int cfd = 0;
    struct sockaddr_in clientaddr;
    int clientaddr_size = sizeof(clientaddr);

    //Infinite loop to accept indefinite number of requests
    for(;;) {
    
        //accept
        if ((cfd = accept(s, (struct sockaddr *) &clientaddr, (socklen_t *) &clientaddr_size)) < 0) //don't care who connects to us; returns client file descriptor
            err_exit(argv[0]);
        

        //int pid = 0;
        //if (pid = fork() == 0) { //child process to handle request
                
            //close(s); //close server listening socket -> will only handle current request and exit    
        

        process_request(cfd, argv[1], clientaddr);   

        //close request
        //if (close(cfd) < 0)
        //  err_exit(argv[0]);
        //exit(0);
        
        //}
        
            
        //Parent process (server)
        if (close(cfd) < 0)
            err_exit(argv[0]);
    }

    exit(0);
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}


