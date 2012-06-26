/*
 *
 * Description: Log Proxy for real-time log viewing
 * Author: Gil Barak
 * Date: 15/06/2012
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>

#define INVALID_HANDLE -1

#define BUFFER_SIZE 256
#define FILE_POLL_INTERVAL 2 // in seconds
#define KEEP_ALIVE_INTERVAL 5 // in seconds


int socket_connected = 0;



void error(const char *msg)
{
    printf("FW LOGGER: %s\r\n", msg);
}


void sigpipe_handler()
{
	error("sigpipe caught!\r\n");
	socket_connected = 0;
}


int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[BUFFER_SIZE + 1] = {0}; // +1 for safety
     char ka_buff;
     struct sockaddr_in serv_addr, cli_addr;
     int n = 0;
     int sent_bytes = 0;
     int fp_fwlog = INVALID_HANDLE;
     struct timeval tv;



     if (argc < 3) {
         fprintf(stderr,"Usage: ./logProxy [listen port] [log file path]\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
        error("opening socket\r\n");
        exit(1);
     }

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     // Bind to local port
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) {
              error("on binding\r\n");
              exit(1);
     }

     listen(sockfd,1);

     signal(SIGPIPE,sigpipe_handler);

     clilen = sizeof(cli_addr);
     while(1)
     {
		 newsockfd = accept(sockfd,
					 (struct sockaddr *) &cli_addr,
					 &clilen);

		 socket_connected = 1;

		 if (newsockfd <= 0) {
			  error("accept\r\n");
			  close(sockfd);
			  exit(1);
		 }

		 printf("Client connected!\r\n");

		 bzero(buffer,BUFFER_SIZE);

		 tv.tv_sec = KEEP_ALIVE_INTERVAL;
		 tv.tv_usec = 0;
		 if(setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv))) {
			 error("failed to set timeout for recv\r\n");
			 close(newsockfd);
			 close(sockfd);
			 exit(1);
		 }


		 // attempt to read the log file every 5 seconds
		 // this is done in case the driver is down and the file is removed
		 do
		 {
			 if(!socket_connected) {
				 error("socket not connected anymore\r\n");
				 break;
			 }

			 n = recv(newsockfd, &ka_buff, 1, 0);

			 if (n < 0) {
				 error("keep alive timeout\r\n");
				 socket_connected = 0;
				 close(newsockfd);
				 if(fp_fwlog != INVALID_HANDLE) {
					 close(fp_fwlog);
					 fp_fwlog = INVALID_HANDLE;
				 }
				 break;
			 }

			 fp_fwlog = open(argv[2], O_RDONLY);
			 sleep(FILE_POLL_INTERVAL);

		 } while(fp_fwlog <= 0);

		 if(socket_connected)
		 {
			 do
			 {
				 if(!socket_connected) {
					 error("socket not connected anymore\r\n");
					 break;
				 }

				 n = read(fp_fwlog,&buffer,BUFFER_SIZE);
				 if (n < 0) {
					 error("reading from file\r\n");
					 if(fp_fwlog != INVALID_HANDLE) {
						 close(fp_fwlog);
						 fp_fwlog = INVALID_HANDLE;
					 }
					 close(newsockfd);
					 break;
				 }

				 sent_bytes = 0;
				 while( n > 0 )
				 {
					 printf("recved = %d\r\n",n);
					 sent_bytes = send(newsockfd,(char *)(buffer + sent_bytes),n,0);
					 if (sent_bytes < 0)
					 {
						 error("Send failed!");
						 break;
					 }
					 n -= sent_bytes;
					 printf("sent_bytes=%d, n=%d\n", sent_bytes,n);
				 }

				 n = recv(newsockfd, &ka_buff, 1, 0);

				 if (n < 0) {
					 error("keep alive timeout\r\n");
					 if(fp_fwlog != INVALID_HANDLE) {
						 close(fp_fwlog);
						 fp_fwlog = INVALID_HANDLE;
					 }
					 close(newsockfd);
					 break;
				 }
			 } while(1);
		 }
     }

     return 0;
}
