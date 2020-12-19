#include <stdio.h>
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp, int clientfd);
void parse(char *uri, char *hostname, char *path, char *port);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

int main(int argc, char **argv)
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    /* Check command line args */
    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }
    //this listenfd now is associated with a IP address and is a server fd
    //Listen for incoming connections on a port which will be specified on the command line
    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
    //wait for connection rquest arrive at, and return a connected descriptor
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); //line:netp:tiny:accept
    
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);

	doit(connfd);
	Close(connfd);
    }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
    int is_static, clientfd, respondlength;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], clientbuf[MAXLINE], servername[MAXLINE], serverport[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE], hostname[MAXLINE], path[MAXLINE], port[5];
    rio_t rio, clientrio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    //read the request line
    readtherequestline:
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    
    if (strcasecmp(method, "GET")) { 
                         //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }
    
    /* Parse URI from GET request */
    if (strstr(uri, "http://") != NULL)
    {
        parse(uri, hostname, path, port);
        printf("1\n");
        printf("after return hostname: %s\n", hostname);
        printf("after return path: %s\n", path);
        printf("after return port: %s\n", port);
        printf("After read and parse\n");
    }
    else{
        printf("invalid http request\n");
        goto readtherequestline;
    }
    
    // Establish connection to the web server, request the object
    clientfd = Open_clientfd(hostname, port);
    if(clientfd<0){
        printf("connection failed\n");
        return;
    }
    Rio_readinitb(&clientrio, clientfd);
    sprintf(clientbuf, "%s %s HTTP/1.0\r\n", method, path);
    Rio_writen(clientfd, clientbuf, strlen(clientbuf));
    sprintf(clientbuf, "Host: %s\r\n", hostname);
    Rio_writen(clientfd, clientbuf, strlen(clientbuf));
    sprintf(clientbuf, "%s\r\n", user_agent_hdr);
    Rio_writen(clientfd, clientbuf, strlen(clientbuf));
    sprintf(clientbuf, "Connection: close\r\n");
    Rio_writen(clientfd, clientbuf, strlen(clientbuf));
    sprintf(clientbuf, "Proxy-Connection: close\r\n");
    Rio_writen(clientfd, clientbuf, strlen(clientbuf));
    printf("1\n");
    read_requesthdrs(&rio, clientfd);
    printf("send request\n");
    sprintf(clientbuf, "lalalalal");
    //read the response
    while ((respondlength = Rio_readlineb(&clientrio, buf, MAXLINE))) {//real server response to buf
        //printf("proxy received %d bytes,then send\n",n);
        Rio_writen(fd, buf, respondlength);  //real server response to real client
    }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
void read_requesthdrs(rio_t *rp, int clientfd) 
{
    printf("read_requesthdrs\n");
    char buf[MAXLINE];
    Rio_readlineb(rp, buf, MAXLINE);
    Rio_writen(clientfd, buf, MAXLINE);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	Rio_readlineb(rp, buf, MAXLINE);
    Rio_writen(clientfd, buf, MAXLINE);
    }
    return;
}

/*
 * parse - parse URI into hostname, path, port
 */
void parse(char* line, char* hostname, char* path, char* port)
{
    char *temphostname;
    char *token;
    char temppath[MAXLINE];
    char *tempport;
    
    temphostname = strstr(line, "//")+2;
    strcpy(temppath, temphostname);
    printf("parse\n");
    token = strtok(temphostname, "/");
    if ((tempport = strstr(token, ":")) == NULL)
    {
        strcpy(port, "80");
        strcpy(hostname, token);
    }
    else
    {
        printf("else: \n");
        strcpy(port, tempport+1);
        temphostname = strtok(token, ":");
        strcpy(hostname, token);
    }
    printf("port: %s\n", port);
    
    printf("hostname: %s\n", hostname);
    strcpy(path, strstr(temppath, "/"));
    //strcpy(path, temppath);
    printf("path: %s\n", path);
}

/*
 * get_filetype - derive file type from file name
 */
void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  
/* $end serve_static */

/*
 * clienterror - returns an error message to the client
 */
/* $begin clienterror */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE];
    
    /* Print the HTTP response headers */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    printf("method:\n");
    sprintf(buf, "Content-type: text/html\r\n\r\n");
    Rio_writen(fd, buf, strlen(buf));

    /* Print the HTTP response body */
    sprintf(buf, "<html><title>Tiny Error</title>");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<body bgcolor=""ffffff"">\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "%s: %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<p>%s: %s\r\n", longmsg, cause);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "<hr><em>The Tiny Web server</em>\r\n");
    Rio_writen(fd, buf, strlen(buf));
}
/* $end clienterror */