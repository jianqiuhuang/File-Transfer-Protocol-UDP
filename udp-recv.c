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
    unsigned long long fileSize = -1;
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen > 0){
        buf[recvlen] = 0;
        strncpy(fileName, buf, strlen(buf));
        if(DEBUG) printf("File name is: %s\n", buf);
    }
    bzero(buf, BUFSIZE);    
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen > 0){
        buf[recvlen] = 0;
        fileSize = (unsigned long long)atoi(buf);
        if(DEBUG) printf("File size is: %llu\n", fileSize);
    }    

    /* check if fileName and fileSize have been set */
    if(fileName == NULL || fileSize == -1){
        perror("Did not receive file name or file size info");
        return 0;
    }

    /* create file stream and allocate temporary data storage array */
    FILE *fp;
    fp = fopen(fileName, "wb");
    if(fp == NULL){
        printf("Error opening file!\n");
        exit(1);
    }
    int arrSize = fileSize / SIZE;
    if(fileSize % SIZE != 0)
        arrSize+=1;

    /* allocating memory for temporary data storage */
    char** stringList = (char**)malloc(arrSize * sizeof(char*));
    for(int i = 0; i < arrSize; ++i){
        stringList[i] = (char*)malloc(SIZE);
    }


	/* now loop, receiving data and printing what we received */
	while(1) {
		printf("waiting on port %d\n", SERVICE_PORT);
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
		if (recvlen > 0) {
			buf[recvlen] = 0;
			printf("received message: \"%s\" (%d bytes)\n", buf, recvlen);
            int index = buf[strlen(buf)-1] - '0';
            buf[strlen(buf)-1] = 0;
            printf("index: %d, %s\n", index, buf);
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
        printf("printing stringlist\n");
        for(int i = 0; i < arrSize; ++i){
            printf("%s\n", stringList[i]);
        }
    }
    fclose(fp);
	/* never exits */
}
