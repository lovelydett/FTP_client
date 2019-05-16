/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */

//@tt: for use of select()
#include    <sys/select.h>  

/* define macros*/
#define MAXBUF	1024
#define STDIN_FILENO	0
#define STDOUT_FILENO	1

/* define FTP reply code */
#define USERNAME	220
#define PASSWORD	331
#define LOGIN		230
#define PATHNAME	257
#define CLOSEDATA	226
#define ACTIONOK	250

/* Define global variables */
char	*host;		/* hostname or dotted-decimal string */
char	*port;
char	*rbuf, *rbuf1;		/* pointer that is malloc'ed */
char	*wbuf, *wbuf1;		/* pointer that is malloc'ed */
struct sockaddr_in	servaddr;

int	    cliopen(char *host, char *port);
void    strtosrv(char *str, char *host, char *port);
void	cmd_tcp(int sockfd);
void	ftp_list(int sockfd);
int	    ftp_get(int sck, char *pDownloadFileName_s);
int	    ftp_put (int sck, char *pUploadFileName_s);

int
main(int argc, char *argv[])
{
	int	fd;

    //@tt: to check if cmd includes ip
	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}

	host = argv[1];
	port = "21";

	
	//1. code here: Allocate the read and write buffers before open().
    //@tt: allocate buffer for rbuf, wbuf... etc



	fd = cliopen(host, port);

	cmd_tcp(fd);

	exit(0);
}


//@tt: establish a tcp connection at host:port
int cliopen(char *host, char *port_c)
{
    printf("\n----------tcp connection establishing---------- \n");
	int port = atoi(port_c);
    int socketCmd;
	if((socketCmd = socket(AF_INET,SOCK_STREAM,0))<0);//@tt: open socket for cmd and get socketID
    {
        printf("error: enable to open socket at: cliopen\n");
        return -1;
    }
    struct hostent *ht = NULL;//@tt: used for acquire servaddr
    ht = gethostbyname(host);
    if(NULL == ht)
    {
        printf("error: enable to get host by name: %s at: cliopen\n",host);
        return -1;
    }
    memset(&servaddr,0,sizeof(struct sockaddr_in));//@tt: clear servaddr
    memcpy(&servaddr.sin_addr.s_addr,(struct in_addr*)ht->h_addr_list,ht->h_length);//@tt: get serveraddr
    
}


/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/
void    strtosrv(char *str, char *host, char *port)
{
	/*************************************************************
	//3. code here
	*************************************************************/
}


/* Read and write as command connection */
void
cmd_tcp(int sockfd)
{
	int		maxfdp1, nread, nwrite, fd, replycode;
	char		host[16];
	char		port[6];
	fd_set		rset;

	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;	/* check descriptors [0..sockfd] */

	for ( ; ; )
	{
		FD_SET(STDIN_FILENO, &rset);
		FD_SET(sockfd, &rset);

		if (select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
			printf("select error\n");
			
		/* data to read on stdin */
		if (FD_ISSET(STDIN_FILENO, &rset)) {
			if ( (nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)
				printf("read error from stdin\n");
			nwrite = nread+5;

			/* send username */
			if (replycode == USERNAME) {
				sprintf(wbuf, "USER %s", rbuf);
				if (write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
			}

			 /*************************************************************
			 //4. code here: send password
			 *************************************************************/


			 /* send command */
			if (replycode==LOGIN || replycode==CLOSEDATA || replycode==PATHNAME || replycode==ACTIONOK)
			{
				/* ls - list files and directories*/
				if (strncmp(rbuf, "ls", 2) == 0) {
					sprintf(wbuf, "%s", "PASV\n");
					write(sockfd, wbuf, 5);
					sprintf(wbuf1, "%s", "LIST -al\n");
					nwrite = 9;
					continue;
				}

				/*************************************************************
				// 5. code here: cd - change working directory/
				*************************************************************/


				/* pwd -  print working directory */
				if (strncmp(rbuf, "pwd", 3) == 0) {
					sprintf(wbuf, "%s", "PWD\n");
					write(sockfd, wbuf, 4);
					continue;
				}

				/*************************************************************
				// 6. code here: quit - quit from ftp server
				*************************************************************/


				/*************************************************************
				// 7. code here: get - get file from ftp server
				*************************************************************/


				/*************************************************************
				// 8. code here: put -  put file upto ftp server
				*************************************************************/


				write(sockfd, rbuf, nread);

			}
		}

		/* data to read from socket */
		if (FD_ISSET(sockfd, &rset)) {
			if ( (nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
				printf("recv error\n");
			else if (nread == 0)
				break;

			/* set replycode and wait for user's input */
			if (strncmp(rbuf, "220", 3)==0 || strncmp(rbuf, "530", 3)==0){
				strcat(rbuf,  "your name: ");
				nread += 12;
				replycode = USERNAME;
			}

			/*************************************************************
			// 9. code here: handle other response coming from server
			*************************************************************/

			/* open data connection*/
			if (strncmp(rbuf, "227", 3) == 0) {
				strtosrv(rbuf, host, port);
				fd = cliopen(host, port);
				write(sockfd, wbuf1, nwrite);
				nwrite = 0;
			}

			/* start data transfer */
			if (write(STDOUT_FILENO, rbuf, nread) != nread)
				printf("write error to stdout\n");
		}
	}

	
	if (close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */
void
ftp_list(int sockfd)
{
	int		nread;

	for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = recv(sockfd, rbuf1, MAXBUF, 0)) < 0)
			printf("recv error\n");
		else if (nread == 0)
			break;

		if (write(STDOUT_FILENO, rbuf1, nread) != nread)
			printf("send error to stdout\n");
	}

	if (close(sockfd) < 0)
		printf("close error\n");
}

/* download file from ftp server */
int	
ftp_get(int sck, char *pDownloadFileName_s)
{
	/*************************************************************
	// 10. code here
	*************************************************************/
}

/* upload file to ftp server */
int 
ftp_put (int sck, char *pUploadFileName_s)
{
	/*************************************************************
	// 11. code here
	*************************************************************/
}