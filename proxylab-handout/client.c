#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "csapp.h"

#define PROGRAM_NAME "client"

/*

Copied helper functions from csapp.h necessary for code to compile

*/

void err_exit() {
	perror(PROGRAM_NAME);
	exit(1);
}

/* $begin unixerror */
void unix_error(char *msg) /* unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}
/* $end unixerror */

void dns_error(char *msg) /* dns-style error */
{
    fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
    exit(0);
}


/*
 * open_clientfd - open connection to server at <hostname, port> 
 *   and return a socket descriptor ready for reading and writing.
 *   Returns -1 and sets errno on Unix error. 
 *   Returns -2 and sets h_errno on DNS (gethostbyname) error.
 */
/* $begin open_clientfd */
int open_clientfd(char *hostname, int port) 
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* check errno for cause of error */

    /* Fill in the server's IP address and port */
    if ((hp = gethostbyname(hostname)) == NULL)
    return -2; /* check h_errno for cause of error */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0], 
      (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* Establish a connection with the server */
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0)
    return -1;
    return clientfd;
}
/* $end open_clientfd */


int Open_clientfd(char *hostname, int port) 
{
    int rc;

    if ((rc = open_clientfd(hostname, port)) < 0) {
    if (rc == -1)
        unix_error("Open_clientfd Unix error");
    else        
        dns_error("Open_clientfd DNS error");
    }
    return rc;
}


/*

main function- This function accepts a URL parameter from the command-line, parses it for data (e.g. domain
name, port, and page on the server), forms an HTTP request with this data, and creates a socket and writes the
request to it. It then reads the server response and prints it, along with its size (in bytes).

*/
int main(int argc, char* argv[]) {
	/* Check for URL argument */
    if (argc != 2) {
		printf("Error: no URL argument was passed.\n");	
		exit(0);
    }

    //Parse URL to get the domain, port (if included), and requested page.
    char* url = argv[1];

    char domain_buf[strlen(url) + 1];	//buffer to hold the parsed domain 
    char port_buf[6];	//buffer to hold parsed port, if included
    char page_buf[strlen(url) + 1]; //holds parsed page, if included
    bzero(domain_buf, strlen(url) + 1);
    bzero(port_buf, 6);
    bzero(page_buf, strlen(url) + 1);
    char* port;
    char* page;

    int index;
    int found_colon = 0;	// 0 = no port, 1 = port
    int colon_count = 0;
    int slash_count = 0;

    for (index = 0; index < strlen(url); index++) {
    	if (url[index] == ':') {	//Increment counts of each special character
    		colon_count++;
    	}
    	else if (url[index] == '/') {
    		slash_count++;
    	}
    	if (colon_count == 2) {		//Check if delimiter is found
    		found_colon = 1;
    		break;
    	}
    	else if (slash_count == 3) {
    		break;
    	}
    	domain_buf[index] = url[index];
    } 

    char* domain = &domain_buf[7];	//truncate "http://" from domain

    //printf("domain is now: %s\n", domain);

    if (index < (strlen(url) - 1)) {
    	index++;	//skip delimiter
    	int offset = index;
    	if (found_colon == 1) { //port specified
    		for (index; index < strlen(url); index++) {
    			if (url[index] == '/') {
    				break;
    			}
				port_buf[(index-offset)] = url[index];
    		}
    	}

    	if (index < (strlen(url) - 1)) {	//page on server is specified
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

    //printf("port is now: %s\n", port);
    //printf("page is now: %s\n", page);

    
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

    //printf("request is: %s\n", request);

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

    printf("Server response: %s\n", resp_buf);
    printf("Response size (bytes): %d\n", strlen(resp_buf));

    /*

    int client_socket = 0;
    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    	err_exit();
    }

    printf("Returned from socket!\n");

    //Construct sockaddr_in structure for connect() call
    struct sockaddr_in clientaddr;
    //bzero(&clientaddr, sizeof(clientaddr)); //0 out all other fields in the struct
    memset(&clientaddr, '\0', sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;  //not placed into packet- no need to check order
    clientaddr.sin_port = htons(atoi(port)); //to convert short (2 bytes) to network byte order

    //Initialize clientaddr.sin_addr
    struct in_addr client_inaddr;
    bzero(&client_inaddr, sizeof(client_inaddr));

    struct addrinfo* client_addrinfo = malloc(sizeof(struct addrinfo));
    struct addrinfo* res_addrinfo = malloc(sizeof(struct addrinfo));
    struct sockaddr_in* temp = malloc(sizeof(struct sockaddr_in));
    bzero(client_addrinfo, sizeof(struct addrinfo));
    bzero(res_addrinfo, sizeof(struct addrinfo));
    bzero(temp, sizeof(struct sockaddr_in));

    client_addrinfo->ai_family = AF_INET;
    client_addrinfo->ai_socktype = SOCK_STREAM;    

    if ((getaddrinfo(domain, NULL, client_addrinfo, &res_addrinfo)) != 0) {
        err_exit();
    }

    printf("Returned from getaddrinfo!\n");

    char* ipaddr = (char *) malloc(20);
    bzero(ipaddr, 20);
    
    //Use first sockaddr struct returned from getaddrinfo()
    temp = (struct sockaddr_in*) res_addrinfo->ai_addr;
    strcpy(ipaddr, inet_ntoa(temp->sin_addr));
    freeaddrinfo(res_addrinfo);
    printf("Server address is: %s\n", ipaddr);

    client_inaddr.s_addr = htonl(atoi(ipaddr));

    //At the end...
    clientaddr.sin_addr = client_inaddr;

    printf("Done creating clientaddr!\n");


    int clientaddr_sz = sizeof(clientaddr);
    socklen_t* clientaddr_size = (socklen_t*) &clientaddr_sz;
    
    if ((connect(client_socket, (struct sockaddr *) &clientaddr, *clientaddr_size)) == -1) {
        err_exit();
    }


    */

    return 0;
}