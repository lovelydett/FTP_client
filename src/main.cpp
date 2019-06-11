/* header files */
#include	<stdio.h>
#include	<stdlib.h>
#include	<netdb.h>	/* getservbyname(), gethostbyname() */
#include	<errno.h>	/* for definition of errno */
#include 	<fcntl.h>	//@tt:for file operates
#include 	<string.h>
#include 	<unistd.h>
#include 	<limits.h>
#include    <thread>
//@tt: for use of select()
#include    <sys/select.h>  
#include 	<sys/socket.h>
#include 	<sys/types.h>
#include 	<sys/stat.h>
#include 	<sys/ioctl.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>

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

enum {LS,GET,PUT}nextCmdMode;

/* Define global variables */
char	*host;		/* hostname or dotted-decimal string */
int		port;
char	*rbuf, *rbuf1;		/* pointer that is malloc'ed */
char	*wbuf, *wbuf1;		/* pointer that is malloc'ed */
struct sockaddr_in	servaddr;//@tt:server's socket address 
int nRet = 0;
char *fileName;

int	    cliopen(char *host, int port);
int     strtosrv(char *str, char *host);//@tt:ip stored in host and port returned
void	cmd_tcp(int sockfd);
void	ftp_list(int sockfd);
int	    ftp_get(int sck, char *pDownloadFileName_s);
int	    ftp_put (int sck, char *pUploadFileName_s);

int main(int argc, char *argv[])
{
	int	fd;
	nRet = socket(AF_INET,SOCK_STREAM,0);
    //@tt: to check if cmd includes ip
	if (0 != argc-2)
	{
		printf("%s\n","missing <hostname>");
		exit(0);
	}

	host = argv[1];
	port = 21;

	
	//1. code here: Allocate the read and write buffers before open().
    //@tt: allocate buffer for rbuf, wbuf... etc
	rbuf = (char*)malloc(MAXBUF);
	wbuf = (char*)malloc(MAXBUF);
	rbuf1 = (char*)malloc(MAXBUF);
	wbuf1 = (char*)malloc(MAXBUF);
	fileName = (char*)malloc(MAXBUF);

	fd = cliopen(host, port);
	if(-1==fd)return -1;

	cmd_tcp(fd);
	free(rbuf);
	free(rbuf1);
	free(wbuf);
	free(wbuf1);
	free(fileName);

	exit(0);
}


//@tt: establish a tcp connection at host:port
int cliopen(char *host, int port)
{
    printf("\n----------tcp connection establishing---------- \n");
    int socketCmd;
	if( -1==(socketCmd = socket(AF_INET,SOCK_STREAM,0)))//@tt: open socket for cmd and get socketID
    {
        printf("error: enable to open socket at: cliopen\n");
        return -1;
    }


    struct hostent *ht = NULL;//@tt: used for acquire servaddr
	char hostname[128];
	hostname[127] = 0;
    // if (gethostname(hostname, 128) == 0)
    //     puts(hostname);
    // else
    //     perror("gethostname");
	// ht = gethostbyname(host);
    // if(NULL == ht)
    // {
    //     printf("error: enable to get host by name: %s at: cliopen\n",host);
    //     return -1;
    // }
    memset(&servaddr,0,sizeof(struct sockaddr_in));//@tt: clear servaddr
    
	//memcpy(&servaddr.sin_addr.s_addr,(in_addr_t*)ht->h_addr_list,ht->h_length);//@tt: get serveraddr
	//memcpy(&servaddr.sin_addr.s_addr,(in_addr_t*)ht->h_addr_list,ht->h_length);//@tt: get serveraddr
    
	servaddr.sin_addr.s_addr = inet_addr(host);
	servaddr.sin_port = htons(port);//@tt:transfer to big-endian
	servaddr.sin_family = AF_INET;
	printf("connecting to server:%s\n",host);
	
	nRet = connect(socketCmd,(struct sockaddr*)&servaddr,sizeof(servaddr));//@tt:force to transfer to sockaddr*
	if(0!=nRet)
	{
		printf("error in tcp connection: nRet = %d \n",nRet);
		return -1;
	}
	printf("----------tcp connection established---------- \n");
	return socketCmd;
}


/*
   Compute server's port by a pair of integers and store it in char *port
   Get server's IP address and store it in char *host
*/
int    strtosrv(char *str, char *host)
{
	unsigned int serverAdd[6];
	int temp_int;
	char temp_str[128];
	char temp;
	sscanf(str,"%d %s %s %s %c%d,%d,%d,%d,%d,%d%c",&temp_int,temp_str,temp_str,temp_str,&temp,&serverAdd[0],&serverAdd[1],&serverAdd[2],&serverAdd[3],&serverAdd[4],&serverAdd[5],&temp);  
	memset(host,0,strlen(host));
	sprintf(host,"%d.%d.%d.%d",serverAdd[0],serverAdd[1],serverAdd[2],serverAdd[3]);
	return serverAdd[4]*256 + serverAdd[5];
}


/* Read and write as command connection */
void cmd_tcp(int sockfd)
{
	int		maxfdp1, nread, nwrite, fd, replycode;
	char	host[16];
	int		port;
	fd_set	rset;

	FD_ZERO(&rset);
	maxfdp1 = sockfd + 1;	/* check descriptors [0..sockfd] */

	while(1)
	{
		FD_SET(STDIN_FILENO, &rset);//@tt:stdin
		FD_SET(sockfd, &rset);//@tt:socketCmd

		nRet = select(maxfdp1, &rset, NULL, NULL, NULL);//@tt:check if rset(s) readable
		if (0>=nRet)
		{
			printf("error: select error, nRet:%d\n",nRet);
		}
		/* data to read on stdin */
		if (FD_ISSET(STDIN_FILENO, &rset)) //@tt:if data readable in stdin
		{
			memset(rbuf,'\0',MAXBUF);
			memset(wbuf,'\0',MAXBUF);
			memset(wbuf1,'\0',MAXBUF);
			if ( (nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)
				printf("read error from stdin\n");
			nwrite = nread+5;

			/* send username */
			if (replycode == USERNAME) //@tt:read username from stdin and send to serverCMD
			{
				sprintf(wbuf, "USER %s", rbuf);
				if (write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
			}

			if(replycode == PASSWORD) //@tt: read password from stdin and send to server
			 /*************************************************************
			 //4. code here: send password
			 *************************************************************/
			{
				sprintf(wbuf, "PASS %s", rbuf);
				if (write(sockfd, wbuf, nwrite) != nwrite)
					printf("write error\n");
			}

			 /* send command */
			if (replycode==LOGIN || replycode==CLOSEDATA || replycode==PATHNAME || replycode==ACTIONOK)
			{
				/* ls - list files and directories*/
				if (strncmp(rbuf, "ls", 2) == 0) 
				{
					sprintf(wbuf, "%s", "PASV\n");
					write(sockfd, wbuf, 5);//@tt:send to server's cmdSock: PASV
					sprintf(wbuf1, "%s", "LIST -al\n");
					nextCmdMode = LS;
					continue;
				}

				/*************************************************************
				// 5. code here: cd - change working directory/
				*************************************************************/
				if (strncmp(rbuf, "cd", 2) == 0) 
				{
					memset(fileName,'\0',strlen(fileName));
					sscanf(rbuf,"cd %s",fileName);
					sprintf(wbuf, "CWD %s", fileName);
					write(sockfd, wbuf, nread);//@tt:send to server's cmdSock: CWD xxxxxx...
					continue;
				}

				/* pwd -  print working directory */
				if (strncmp(rbuf, "pwd", 3) == 0) 
				{
					sprintf(wbuf, "%s", "PWD\n");
					write(sockfd, wbuf, 4);
					continue;
				}

				/*************************************************************
				// 6. code here: quit - quit from ftp server
				*************************************************************/
				if (strncmp(rbuf, "quit", 4) == 0) 
				{
					sprintf(wbuf, "%s", "QUIT\n");
					write(sockfd, wbuf, 5);
					nRet = close(sockfd);
					if(nRet<0)
					{
						printf("error: failed to close sock:%d\n",sockfd);
					}
					break;
				}


				/*************************************************************
				// 7. code here: get - get file from ftp server
				*************************************************************/
				if (strncmp(rbuf, "get", 3) == 0) 
				{
					sprintf(wbuf, "%s", "PASV\n");
					memset(fileName,'\0',strlen(fileName));
					sscanf(rbuf,"get %s",fileName);
					sprintf(wbuf1, "RETR %s", fileName);
					write(sockfd,wbuf,5);//@tt:get connected in PASV mode
					nextCmdMode = GET;
					continue;
				}

				/*************************************************************
				// 8. code here: put -  put file upto ftp server
				*************************************************************/
				if (strncmp(rbuf, "put", 3) == 0) 
				{
					sprintf(wbuf, "%s", "PASV\n");
					memset(fileName,'\0',strlen(fileName));
					sscanf(rbuf,"put %s",fileName);
					sprintf(wbuf1, "STOR %s", fileName);//@tt:set next cmd line
					write(sockfd,wbuf,5);//@tt:get connected in PASV mode
					nextCmdMode = PUT;
					continue;
				}

				else
				{
					printf("\n error: wrong cmd line. \n");
					continue;
				}
				

			}
		}

		/* data to read from socket */
		if (FD_ISSET(sockfd, &rset)) {
			if ( (nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
			{
				printf("recv error\n");
				continue;
			}
			else if (nread == 0)
				continue;
			write(STDOUT_FILENO, rbuf, strlen(rbuf));
			/* set replycode and wait for user's input */
			if (strncmp(rbuf, "220", 3)==0 || strncmp(rbuf, "530", 3)==0){
				write(STDOUT_FILENO,"your name: ",strlen("your name: "));
				//nread += 12;
				replycode = USERNAME;
			}

			/*************************************************************
			// 9. code here: handle other response coming from server
			*************************************************************/

			/* open data connection*/
			if (strncmp(rbuf, "227", 3) == 0) {
				//printf("A: %s\n",rbuf);
				port = strtosrv(rbuf, host);
				fd = cliopen(host, port);
				switch (nextCmdMode)
				{
				case LS:
					if(write(sockfd, wbuf1, strlen(wbuf1))!=strlen(wbuf1))
					{
						printf("error: failed to send cmd to serversock");
					}
					ftp_list(fd);
					break;
				case GET:
				{
					if(write(sockfd, wbuf1, strlen(wbuf1))!=strlen(wbuf1))
					{
						printf("error: failed to send cmd to serversock");
					}
					std::thread *t = new std::thread(ftp_get,fd,fileName);
					t->detach();
					break;
				}
				case PUT:
				{
					if(write(sockfd, wbuf1, strlen(wbuf1))!=strlen(wbuf1))
					{
						printf("error: failed to send cmd to serversock");
					}
					std::thread *t = new std::thread(ftp_put,fd,fileName);
					t->detach();
					break;	
				}			
				default:
					break;
				}
				nwrite = 0;
			}
			if(strncmp(rbuf,"331",3) == 0)
			{
				write(STDOUT_FILENO,"your password:",strlen("your password:"));
				nread += 16;
 				replycode = PASSWORD;
			}
			if(strncmp(rbuf,"230",3) == 0)
			{
 				replycode = LOGIN;
			}
			if(strncmp(rbuf,"257",3) == 0)
			{
 				replycode = PATHNAME;
			}
			if(strncmp(rbuf,"250",3) == 0)
			{
 				replycode = ACTIONOK;
			}
			if(strncmp(rbuf,"226",3) == 0)
			{
 				replycode = CLOSEDATA;
			}
			/* start data transfer */

		}
	}

	
	if (close(sockfd) < 0)
		printf("close error\n");
}


/* Read and write as data transfer connection */
void ftp_list(int sockfd)
{
	int		nread;

	for ( ; ; )
	{
		/* data to read from socket */
		if ( (nread = recv(sockfd, rbuf1, MAXBUF, 0)) < 0)
		{
			printf("recv error\n");
			break;
		}
		else if (nread == 0)
			break;
		if (write(STDOUT_FILENO, rbuf1, nread) != nread)
			printf("send error to stdout\n");
	}
	
	if (close(sockfd) < 0)
		printf("close error\n");
}

/* download file from ftp server */
int	ftp_get(int sck, char *pDownloadFileName_s)
{

	/*************************************************************
	// 10. code here
	*************************************************************/
	
	int file_fd = open(pDownloadFileName_s,O_WRONLY|O_CREAT|O_TRUNC);//@tt:open file ready to write in
	if(-1 == file_fd)
	{
		printf("error: file open failed, unable to get file from server");
		return -1;
	}
	memset(rbuf1,0,sizeof(rbuf1));
	int readCount;
	while(1)
	{
		readCount = recv(sck,rbuf1,MAXBUF,0);
		if(0 == readCount)
		{
			printf("reading finished");
			break;
		}
		if(0>readCount)
		{
			printf("error: unable to read from sck:%d\n",sck);
			break;
		}
		nRet = write(file_fd,rbuf1,readCount);
		if(nRet!=readCount)
		{
			printf("error: failed to write to file:%s\n",pDownloadFileName_s);
			return -1;
		}
	}
	//@tt: get file finished
	nRet = close(sck);
	if(0>nRet)
	{
		printf("error: failed to close socket, nRet:%d\n",nRet);
		return -1;
	}
	return 0;
}

/* upload file to ftp server */
int ftp_put (int sck, char *pUploadFileName_s)
{
	/*************************************************************
	// 11. code here
	*************************************************************/
	int file_fd = open(pUploadFileName_s,O_RDONLY);//@tt:open file ready to write in
	if(-1 == file_fd)
	{
		printf("error: file open failed, unable to get file from server");
		return -1;
	}
	memset(wbuf1,0,sizeof(wbuf1));
	int readCount;
	while(1)
	{
		readCount = read(file_fd,wbuf1,MAXBUF);
		if(0 == readCount)
		{
			printf("reading finished from file:%s\n",pUploadFileName_s);
			break;
		}
		if(0>readCount)
		{
			printf("error: unable to read from file:%s\n",pUploadFileName_s);
			break;
		}
		nRet = write(sck,wbuf1,readCount);
		if(nRet!=readCount)
		{
			printf("error: failed to write to socket:%d\n",sck);
			return -1;
		}
	}
	//@tt: put file finished
	nRet = close(sck);
	if(0>nRet)
	{
		printf("error: failed to close socket, nRet:%d\n",nRet);
		return -1;	
	}
	return 0;
}