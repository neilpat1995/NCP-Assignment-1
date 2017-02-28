/*
 * proxy.c - A Simple Sequential Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 2
 * Student Name: Neil M. Patel
 * 

This program implements a simple iterative web proxy. Its functionality is as follows:

1. On program execution, the proxy creates a socket (using socket()), binds that socket to a port specified by
the user as a command-line argument (using bind()), sets up the socket to listen for requests (using 
listen()), and waits for client connections (using accept()). Upon returning from accept() (i.e. getting a 
connection to a client), processing of the request begins in process_request().

2. Upon returning from accept(), the proxy first reads the request, ensures that it is well-formed (i.e.
contains a URI), and prints it to the terminal (stage 1).

3. The proxy then parses the URI from the request and fetches the client IP address, and prints these to the
terminal as well. It also logs this information using the provided format_log_entry() function to the proxy.log
logfile (stage 2).

4. The proxy then parses the domain name, port (if specified), and page (if specified) from the URI, forms an
HTTP request based on this data, creates and connects to a socket to communicate with the server, and writes
the request to this new socket. It then reads the response from the same socket and writes it to the socket
created to communicate with the client for viewing in the browser.

5. After processing this request, the client file descriptor used to communicate with the proxy is closed, and
the proxy calls accept() again to wait for the next client.

Note: Stage 3 of the assignment is implemented in the separate client.c file, which is passed a URI and prints the
HTML response from the corresponding server.

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
    //read request
    char reqbuf[10000];
    bzero(reqbuf, sizeof(reqbuf));
    int reqlen = 0;
    if ( (reqlen = read(cfd, reqbuf, sizeof(reqbuf))) == -1 ) {
        err_exit();
    }
    reqbuf[reqlen] = '\0';
    printf("Request read is: %s\n", reqbuf);

    int req_token_counter = 0;
    char reqbufcpy[strlen(reqbuf)];
    bzero(reqbufcpy, sizeof(reqbufcpy));
    strcpy(reqbufcpy, reqbuf);

    if(strtok(reqbufcpy, " ") != NULL) {
        req_token_counter++;
    }

    while (strtok(NULL, " ") != NULL) {
        req_token_counter++;
    }

    printf("Request token counter = %d\n", req_token_counter);

    if (req_token_counter >= 3) {

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
        fclose(fp);

        //Parse URL to get the domain, port (if included), and requested page.
        char* url = request_uri;

        char domain_buf[strlen(url) + 1];   //buffer to hold the parsed domain 
        char port_buf[6];   //buffer to hold parsed port, if included
        char page_buf[strlen(url) + 1]; //holds parsed page, if included
        bzero(domain_buf, strlen(url) + 1);
        bzero(port_buf, 6);
        bzero(page_buf, strlen(url) + 1);
        char* port;
        char* page;

        int index;
        int found_colon = 0;    // 0 = no port, 1 = port
        int colon_count = 0;
        int slash_count = 0;

        for (index = 0; index < strlen(url); index++) {
            if (url[index] == ':') {    //Increment counts of each special character
                colon_count++;
            }
            else if (url[index] == '/') {
                slash_count++;
            }
            if (colon_count == 2) {     //Check if delimiter is found
                found_colon = 1;
                break;
            }
            else if (slash_count == 3) {
                break;
            }
            domain_buf[index] = url[index];
        } 

        char* domain = &domain_buf[7];  //truncate "http://" from domain

        printf("domain is now: %s\n", domain);

        if (index < (strlen(url) - 1)) {
            index++;    //skip delimiter
            int offset = index;
            if (found_colon == 1) { //port specified
                for (index; index < strlen(url); index++) {
                    if (url[index] == '/') {
                        break;
                    }
                    port_buf[(index-offset)] = url[index];
                }
            }

            if (index < (strlen(url) - 1)) {    //page on server is specified
                index = (found_colon == 1)? index + 1: index; //skip delimiter if port was specified
                int page_offset = index;
                for (index; index < strlen(url); index++) {
                    page_buf[(index - page_offset)] = url[index];
                }
            }
        }

        page = (char*) malloc(sizeof(page_buf) + 1);
        bzero(page, sizeof(page_buf) + 1);
        strcpy(page, "/");

        port = (port_buf[0] == '\0')? "80": port_buf;
        page = (page_buf[0] == '\0')? page: strcat(page, page_buf);

        printf("port is now: %s\n", port);
        printf("page is now: %s\n\n", page);

        
        int clientfd = 0;
        if((clientfd = Open_clientfd(domain, atoi(port))) < 0) {
            err_exit();
        }

        //Form request
        const char* http_method = "GET ";
        const char* http_version = " HTTP/1.0\r\n\r\n";

        char request[strlen(http_method) + strlen(page) + strlen(http_version)];
        strcpy(request, http_method);
        strcat(request, page);
        strcat(request, http_version);

        //Send request
        int write_resp = 0;
        if ((write_resp = write(clientfd, request, sizeof(request))) <= 0) {
            if (write_resp == 0) {
                printf("write() call wrote 0 bytes to socket!\n");
            }
            else {
                err_exit();
            }
        }

        //Read response
        char resp_buf[100000];
        int offset = 0;
        int read_res = 0;
        while(offset < (sizeof(resp_buf) - 1)) {
            if((read_res = read(clientfd, &resp_buf[offset], 1)) <= 0) {
                if(read_res == 0) {
                    break;
                }
                else {
                    err_exit();
                }
            }
            offset++;
        }
        
        resp_buf[offset] = '\0';

        //printf("Server response: %s\n", resp_buf);
        //printf("Response size (bytes): %d\n", strlen(resp_buf));

        int browser_resp = 0;

        if((browser_resp = write(cfd, resp_buf, strlen(resp_buf))) <= 0) {
            if (browser_resp == 0) {
                printf("Writing result to web browser returned 0 bytes!\n");
            }
            else {
                err_exit();
            }
        }

    }

    
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
    if ((listen(s, 100)) < 0) //2nd param specifies max number of pending connections allowed
        err_exit(argv[0]);
    
    int cfd = 0;
    struct sockaddr_in clientaddr;
    int clientaddr_size = sizeof(clientaddr);

    //Infinite loop to accept indefinite number of requests
    for(;;) {
    
        //accept
        if ((cfd = accept(s, (struct sockaddr *) &clientaddr, (socklen_t *) &clientaddr_size)) < 0) //don't care who connects to us; returns client file descriptor
            err_exit(argv[0]);  

        process_request(cfd, argv[1], clientaddr);

        //Request served
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


