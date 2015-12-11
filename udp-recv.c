/*
    Demo-udp-03: udp-recv: a simple udp server
	receive udp messages

        usage:  udp-recv

        Paul Krzyzanowski
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "port.h"

#define BUFSIZE 512
#define DEBUG 1
#define SIZE 5
int
main(int argc, char **argv)
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	int msgcnt = 0;			/* count # of messages we received */
	unsigned char buf[BUFSIZE];	/* receive buffer */


	/* create a UDP socket */

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket\n");
		return 0;
	}

	/* bind the socket to any valid IP address and a specific port */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
    //bind to a specified address and port number
//    myaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
//    myaddr.sin_port = htons(1333);
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(SERVICE_PORT);

	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

    /* receive file name and file size */
    char fileName[100];
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen > 0){
        buf[recvlen] = 0;
        strncpy(fileName, buf, strlen(buf));
    }
    bzero(buf, BUFSIZE);    

    /* check if fileName have been set */
    if(fileName == NULL){
        perror("Did not receive file name");
        return 0;
    }

    /* create file stream and allocate temporary data storage array */
    FILE *fp;
    fp = fopen(fileName, "wb");
    if(fp == NULL){
        perror("Error opening file!");
        exit(1);
    }


	/* now loop, receiving data and printing what we received */
	while(1) {
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			buf[recvlen] = 0;
			printf("received message: \"%s\" (%d bytes)\n", buf, recvlen);
            int index = buf[strlen(buf)-1] - '0';
            buf[strlen(buf)-1] = 0;
            printf("what's write to file: %s\n", buf);
            //strncpy(stringList[buf[SIZE-1] - '0'], buf, SIZE)
            fwrite(buf, SIZE, 1, fp);
		}
		else{
			//printf("uh oh - something went wrong!\n");
            break;
        }
		sprintf(buf, "ack %d", msgcnt++);
		printf("sending response \"%s\"\n", buf);
		if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
			perror("sendto");
    }
    fclose(fp);
    close(fd);
	/* never exits */
}
