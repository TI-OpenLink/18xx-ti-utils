/*
 *
 * Description: Log Proxy for real-time log viewing
 * Author: Gil Barak
 * Date: 15/05/2012
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

#define BUFFER_SIZE 1024

void error(const char *msg)
{
    perror(msg);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[BUFFER_SIZE];
     char ka_buff;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     int fp_fwlog;
     struct timeval tv;

     if (argc < 3) {
         fprintf(stderr,"Usage: ./logProxy [listen port] [log file path]\n");
         exit(1);
     }

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
        error("ERROR opening socket");
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
              error("ERROR on binding");
              exit(1);
     }

     listen(sockfd,1);

     clilen = sizeof(cli_addr);
     while(1)
     {
		 newsockfd = accept(sockfd,
					 (struct sockaddr *) &cli_addr,
					 &clilen);

		 if (newsockfd < 0) {
			  error("ERROR on accept");
			  close(sockfd);
			  exit(1);
		 }

		 bzero(buffer,BUFFER_SIZE);

		 tv.tv_sec = 1;
		 tv.tv_usec = 0;
		 if(setsockopt(newsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv))) {
			 error("ERROR failed to set timeout for recv");
			 close(newsockfd);
			 close(sockfd);
			 exit(1);
		 }


		 fp_fwlog = open(argv[2], O_RDONLY);
		 if (fp_fwlog < 0) {
			 error("ERROR opening log file");
			 close(newsockfd);
			 close(sockfd);
		 	 exit(1);
		 }

		 do
		 {
			 n = read(fp_fwlog,&buffer,BUFFER_SIZE);
			 if (n < 0) {
				 error("ERROR reading from file");
				 close(newsockfd);
				 close(fp_fwlog);
				 break;
			 }

			 if(n>0)
				 n = send(newsockfd,buffer,n,0);

			 n = recv(newsockfd, &ka_buff, 1, 0);

			 if (n < 0) {
				 error("ERROR keep alive timeout");
				 close(newsockfd);
				 close(fp_fwlog);
				 break;
			 }
		 } while(1);
     }

     return 0;
}
