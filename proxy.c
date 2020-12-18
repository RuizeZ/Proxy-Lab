#include <stdio.h>
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse(char *uri, char *hostname, char *path, char *port);
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

	doit(connfd);                                             //line:netp:tiny:doit
	Close(connfd);                                            //line:netp:tiny:close
    }
}

/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd)
{
    int is_static, clientfd;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE], clientbuf[MAXLINE], servername[MAXLINE], serverport[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE], hostname[MAXLINE], path[MAXLINE], port[5];
    rio_t rio, clientrio;

    /* Read request line and headers */
    Rio_readinitb(&rio, fd);
    //read the request line
    if (!Rio_readlineb(&rio, buf, MAXLINE))  //line:netp:doit:readrequest
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
    if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
        clienterror(fd, method, "501", "Not Implemented",
                    "Tiny does not implement this method");
        return;
    }

    //get rquested name and port
    Rio_readlineb(&rio, buf, MAXLINE);
    printf("%s", buf);
    if(strcmp(buf, "\r\n")) {
        sscanf(buf, "Host: %[^:]%*c%s", servername, serverport); 
        printf("servername: %s\n", servername);
        printf("serverport: %s\n", serverport);  
        read_requesthdrs(&rio);   
    }

    /* Parse URI from GET request */
    is_static = parse(uri, hostname, path, port);       //line:netp:doit:staticcheck
    printf("after return hostname: %s\n", hostname);
    printf("after return path: %s\n", path);
    printf("after return port: %s\n", port);
    // if (stat(path, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
	// clienterror(fd, path, "404", "Not found",
	// 	    "Tiny couldn't find this file");
	// return;
    // }                                                    //line:netp:doit:endnotfound
    printf("After read and parse\n");
    // Establish connection to the web server, request the object
    clientfd = open_clientfd(servername, serverport);
    
    Rio_readinitb(&clientrio, clientfd);
    printf("1\n");
    while (Fgets(clientbuf, MAXLINE, stdin) != NULL)
    {
        printf("2\n");
        sprintf(clientbuf, "%s %s %s\r\n", method, servername, version);
        Rio_writen(clientfd, clientbuf, strlen(clientbuf));
        Rio_readlineb(&clientrio, clientbuf, MAXLINE);
        Fputs(clientbuf, stdout);
        
    }

    Close(clientfd);
    printf("send request\n");
    exit(0);
    






    if (is_static) { /* Serve static content */          
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't read the file");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
    }
    else { /* Serve dynamic content */
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
	    clienterror(fd, filename, "403", "Forbidden",
			"Tiny couldn't run the CGI program");
	    return;
	}
	serve_dynamic(fd, filename, cgiargs);            //line:netp:doit:servedynamic
    }
}

/*
 * read_requesthdrs - read HTTP request headers
 */
void read_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}

/*
 * parse_uri - parse URI into filename and CGI args
 *             return 0 if dynamic content, 1 if static
 */
int parse(char* line, char* hostname, char* path, char* port)
{
    char *temphostname;
    char *token;
    char *temppath;
    char *tempport;
    temphostname = strstr(line, "www");
    strcpy(temppath, temphostname);
    token = strtok(temphostname, "/");
    if ((tempport = strstr(token, ":")) == NULL)
    {
        strcpy(port, "80");
    }
    else
    {
        printf("else: \n");
        strcpy(port, tempport+1);
    }

    printf("port: %s\n", port);
    strcpy(hostname, token);
    printf("hostname: %s\n", hostname);
    temppath = strstr(temppath, "/");
    strcpy(path, temppath);
    printf("path: %s\n", path);

}
// int parse_uri(char *uri, char *filename, char *cgiargs) 
// {
//     char *ptr;

//     if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
// 	strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
// 	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
// 	strcat(filename, uri);                           //line:netp:parseuri:endconvert1
// 	if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
// 	    strcat(filename, "home.html");               //line:netp:parseuri:appenddefault
// 	return 1;
//     }
//     else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
// 	ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
// 	if (ptr) {
// 	    strcpy(cgiargs, ptr+1);
// 	    *ptr = '\0';
// 	}
// 	else 
// 	    strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
// 	strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
// 	strcat(filename, uri);                           //line:netp:parseuri:endconvert2
// 	return 0;
//     }
// }

/*
 * serve_static - copy a file back to the client 
 */
/* $begin serve_static */
void serve_static(int fd, char *filename, int filesize)
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];

    /* Send response headers to client */
    get_filetype(filename, filetype);    //line:netp:servestatic:getfiletype
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); //line:netp:servestatic:beginserve
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n", filesize);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: %s\r\n\r\n", filetype);
    Rio_writen(fd, buf, strlen(buf));    //line:netp:servestatic:endserve

    /* Send response body to client */
    srcfd = Open(filename, O_RDONLY, 0); //line:netp:servestatic:open
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0); //line:netp:servestatic:mmap
    Close(srcfd);                       //line:netp:servestatic:close
    Rio_writen(fd, srcp, filesize);     //line:netp:servestatic:write
    Munmap(srcp, filesize);             //line:netp:servestatic:munmap
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
 * serve_dynamic - run a CGI program on behalf of the client
 */
/* $begin serve_dynamic */
void serve_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    /* Return first part of HTTP response */
    sprintf(buf, "HTTP/1.0 200 OK\r\n"); 
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: Tiny Web Server\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) { /* Child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
    }
    Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}
/* $end serve_dynamic */

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