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
#define DATASIZE 508
#define SEQNUMSIZE 4
int
main(int argc, char **argv)
{
	struct sockaddr_in myaddr;	/* our address */
	struct sockaddr_in remaddr;	/* remote address */
	socklen_t addrlen = sizeof(remaddr);		/* length of addresses */
	int recvlen;			/* # bytes received */
	int fd;				/* our socket */
	char buf[BUFSIZE], ackBuf[BUFSIZE];	/* receive buffer */


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

    printf("before binding\n");
	if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}

    printf("receiving file name\n");
    /* receive file name and file size */
    char fileName[100];
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    if (recvlen > 0){
        buf[recvlen] = 0;
        strncpy(fileName, buf, strlen(buf));
    }else{
        perror("file name is not received");
    }
    printf("file name received\n");
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
    printf("file created: %s\n", fileName);

	/* now loop, receiving data and printing what we received */
    const int WS = 4;
    int LFR = 0, LFA = WS;
    unsigned int seqNum = 0;
    int window[WS];
    /* initialize all element to zero */
    for(int i = 0; i < WS; ++i)
        window[i] = 0;

	while(1) {
        memset(buf, 0, BUFSIZE);    
        /* packet data to buf */
		recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
        printf("received length: %d\n", recvlen);

        /* parse seqNum */
        if(recvlen > SEQNUMSIZE){
            uint32_t networkByteI = 0;
            memcpy(&networkByteI, buf+recvlen-SEQNUMSIZE, SEQNUMSIZE);
            seqNum = ntohl(networkByteI);
            printf("received seqNum: %d\n", seqNum);
        }
        /* empty packet --- end of file */
        else if (recvlen > 0){
            break;
        }else{
            perror("no data in buf from recvfrom");
            exit(1);
        }

/*******  Selective repeat protocol ******/

        /* check if pointers are within bounds */
        if(LFA - LFR > WS){
            perror("pointers out of bound");
            exit(1);
        }
        
        /* accept packet;; write to file and send acknowledgement */
        if(LFR < seqNum && seqNum <= LFA){
            /* keep track of packets within the window */
            int index = (seqNum - 1) % WS;
            
            /* packet previous received;; resend acknowledgement */
            if(window[index] == seqNum){
                /* sending ack to sender */    
                memset(ackBuf, 0, BUFSIZE);
	    	    sprintf(ackBuf, "ack %d", seqNum);
	    	    printf("Resending response \"%s\"\n", ackBuf);
    		    if (sendto(fd, ackBuf, strlen(ackBuf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
		    	    perror("sendto");

            }
            /* new packet within windown range;; update windown array and variables */
            else{
                window[index] = seqNum;
            
                /* update variables and the window */
                int i;
                if(LFR == 0) i = 0;
                else i = (LFR) % WS;
                for(; i < WS; i = (i+1) % WS){
                    if(window[i] == LFR+1){
                        ++LFR;
                    }else
                        break;
                }     
                LFA = LFR + WS;
                printf("LFA: %d;; LFR: %d\n", LFA, LFR);
                /* Get byte position in output file for write */
                fseek(fp, (seqNum-1)*DATASIZE, SEEK_SET);
                /* Right to the specified location */
                fwrite(buf, 1, recvlen-SEQNUMSIZE, fp);
        
                /* sending ack to sender */    
                memset(ackBuf, 0, BUFSIZE);
	    	    sprintf(ackBuf, "ack %d", seqNum);
	    	    printf("sending response \"%s\"\n", ackBuf);
    		    if (sendto(fd, ackBuf, strlen(ackBuf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
		    	    perror("sendto");
            }
        }
        
        /* resend packet acknowlegement because seqNum <= LFR */
        else if(seqNum <= LFR){
            memset(ackBuf, 0, BUFSIZE);
		    sprintf(ackBuf, "ack %d", seqNum);
		    printf("Resending response \"%s\"\n", ackBuf);
		    if (sendto(fd, ackBuf, strlen(ackBuf), 0, (struct sockaddr *)&remaddr, addrlen) < 0)
			    perror("sendto");
        }
        
        /* discard the packet */
        else{
            
        }
	        /* print to check what is in window */
            for(int i = 0; i < WS; ++i){
                printf("window[%d] = %d\n", i, window[i]);
            }
    }
    fclose(fp);
    close(fd);
	/* never exits */
}
