/*
        demo-udp-03: udp-send: a simple udp client
	send udp messages
	This sends a sequence of messages (the # of messages is defined in MSGS)
	The messages are sent to a port defined in SERVICE_PORT 

        usage:  udp-send

        Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "port.h"

#define BUFLEN 512
#define MSGS 5	/* number of messages to send */
#define DEBUG 1
#define SIZE 5
int main(void)
{
	struct sockaddr_in myaddr, remaddr;
	int fd, i, slen=sizeof(remaddr);
	unsigned char buf[BUFLEN];	/* message buffer */
	int recvlen;		/* # bytes in acknowledgement message */
	char *server = "127.0.0.1";	/* change this to use a different server */

	/* create a socket */

	if ((fd=socket(AF_INET, SOCK_DGRAM, 0))==-1)
		printf("socket created\n");

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

    /* open the file */
    FILE *fp;
    fp = fopen("in.txt", "rb");
    if(fp == NULL){
        perror("fopen");
        exit(1);
    }
    /* determine file size */
    fseek(fp, 0L, SEEK_END);
    unsigned long long fileSize  = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if(DEBUG) printf("File size: %d\n", fileSize);

	/* now let's send the messages */

    /* first send the file name */
    sprintf(buf, "in.txt");
    if(sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1){
        perror("sendto");
        exit(1);
    }
    /* send file size */
    sprintf(buf, "%llu", fileSize);
    if(sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1){
        perror("sendto");
        exit(1);
    }

    /* transmit file data */
    i = 0;
    bzero(buf, BUFLEN);
    // integer argument for reading length n-1
    while(1){
	    fread(buf, SIZE, 1, fp);
        if(strlen(buf) == 0){
            printf("size zero\n");
            break;
        }
        if(DEBUG){
//            printf("*****Before sprintf, buf contains: %s\n", buf);
//		    printf("*****Sending packet %d to %s port %d\n", i, server, SERVICE_PORT);
        }
//		sprintf(buf, "%s%d", buf, i);
        /* append sequence number to end of message */
        buf[strlen(buf)] = i + '0';

        if(DEBUG)
            printf("*****After sprintf, buf contains: %s\n", buf);
		
        if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
			perror("sendto");
			exit(1);
		}
        
        bzero(buf, BUFLEN);
        
        /* now receive an acknowledgement from the server */
		recvlen = recvfrom(fd, buf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
                if (recvlen >= 0) {
                        buf[recvlen] = 0;	/* expect a printable string - terminate it */
                        printf("received message: \"%s\"\n", buf);
                }
        
        bzero(buf, BUFLEN);
        i++;
	
    }
    /* final empty packet so that the server knows the transmission is done */
    if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1) {
	    perror("sendto");
		exit(1);
	}

	close(fd);
	return 0;
}
