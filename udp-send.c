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
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "port.h"

#define BUFLEN 512
#define DEBUG 1
#define DATASIZE 508
#define SEQNUMSIZE 4
#define WINDOWSIZE 16
#define WAITLIMIT 2


int inet_aton(const char *cp, struct in_addr *inp);

struct sentFrame{
	uint32_t seqNum;
	int resend;
	int armed;
	//Timing object here
	time_t timing;
};

int main(int argc, char **argv)
{
    if(argc != 4){
        perror("usage: ./upd-send <file-path> <file-name> <server-ip>");
        exit(1);
    }

    struct sockaddr_in myaddr, remaddr;
	int fd, slen=sizeof(remaddr);
	char buf[BUFLEN], ackBuf[BUFLEN];	/* message buffer */
	int recvlen;		/* # bytes in acknowledgement message */
	char *server = argv[3];	/* change this to use a different server */
	
	//initialize the window structure
	struct sentFrame window[WINDOWSIZE];
	for(int j = 0; j < WINDOWSIZE; j++){
		window[j].seqNum = -1;
		window[j].resend = 0;
		window[j].armed = 0;
	}
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
    char fileCompleteName[100];
    strcpy(fileCompleteName, argv[1]);
    strcat(fileCompleteName, argv[2]);

	FILE *fp;
	fp = fopen(fileCompleteName, "rb");
	if(fp == NULL){
		perror("fopen");
		exit(1);
	}

	/* now let's send the messages */

	/* first send the file name */
	sprintf(buf, "%s", argv[2]);
	if(sendto(fd, buf, strlen(buf), 0, (struct sockaddr *)&remaddr, slen)==-1){
		perror("sendto");
		exit(1);
	}

	int finished = 0;
	int remainingPackets = 0;

	printf("fileName: %s\n", buf);
	/* transmit file data */
	unsigned int i = 1;
	// integer argument for reading length n-1

	while(1){
		memset(buf, 0, BUFLEN);
		for(int j = 0; j < WINDOWSIZE; j++){
			//check if any frames are marked for resend
			if(window[j].armed == 1 && window[j].resend == 1){
				uint32_t numToResend = window[j].seqNum;
				printf("ACK not received in time for %d, resending\n", numToResend);
				
				fseek(fp, (numToResend - 1)*DATASIZE, SEEK_SET);
				int nRead = fread(buf, 1, DATASIZE, fp);
				printf("number of byes read %d\n", nRead);
				
				uint32_t networkByteI = htonl(i);
				//append 4 byte seqNum to the end of buf
				memcpy(buf+nRead, &networkByteI, SEQNUMSIZE);

				if (sendto(fd, buf, nRead+SEQNUMSIZE, 0, (struct sockaddr *)&remaddr, slen)==-1) {
						perror("sendto");
						exit(1);
				}	

				//Arm the timer
				window[j].timing = time(NULL);
				window[j].resend = 0;
			}	
		}
		
		memset(buf, 0, BUFLEN);
		for(int j = 0; j < WINDOWSIZE; j++){
			if(window[j].armed == 0){
				fseek(fp, (i - 1)*DATASIZE, SEEK_SET);
				int nRead = fread(buf, 1, DATASIZE, fp);
				if(nRead == 0){
					printf("Sending final packet\n");
					finished = 1;
					window[j].armed = 0;
				}
				else{
					window[j].seqNum = i;
					window[j].resend = 0;
					window[j].armed = 1;
					window[j].timing = time(NULL);
				}
				printf("number of bytes read: %d\n", nRead);
				
				uint32_t networkByteI = htonl(i);

				//append 4 byte seqNum to the end of buf
				memcpy(buf+nRead, &networkByteI, SEQNUMSIZE);

				printf("transmitting number of bytes: %d\n", nRead + SEQNUMSIZE);
				//break when eof
				//transmit one last empty packet

				if((sendto(fd, buf, nRead+SEQNUMSIZE, 0, (struct sockaddr *)&remaddr, slen)==-1)) {
					perror("sendto");
					exit(1);
				}	
				
				break;
			}
		}

		for(int j = 0; j < WINDOWSIZE; j++){
			if(window[j].armed == 1){
				if(difftime(time(NULL), window[j].timing) > WAITLIMIT){
					window[j].resend = 1;
				}
			}
		}

		if(finished == 1){
			printf("Sent last packet, looking for packets waiting for a response\n");
			for(int j = 0; j < WINDOWSIZE; j++){
				if(window[j].armed == 1){
					printf("%d is still waiting for a response\n", window[j].seqNum);
					remainingPackets = 1;
					break;
				}
			}
		}
		if(finished == 1 && remainingPackets == 0){
			printf("All packets accounted for, exiting\n");
			break;
		}

		/* now receive an acknowledgemen from the server */
		recvlen = recvfrom(fd, ackBuf, BUFLEN, 0, (struct sockaddr *)&remaddr, &slen);
		if (recvlen >= 0) {
			printf("received message: \"%s\"\n", ackBuf);
		//	unsigned int networkByteI = 0;
		//	printf("before memcpy\n");
		//	memcpy(&networkByteI, ackBuf, SEQNUMSIZE);
		//	printf("after memcpy\n");
		//	unsigned int ackSeqNum = ntohl(networkByteI);
		//	printf("after ntohl %u\n", ackSeqNum);
			unsigned int ackSeqNum = 0;
			sscanf(ackBuf, "%u", &ackSeqNum);
			printf("converted to %u\n" , ackSeqNum);
			for(int j = 0; j < WINDOWSIZE; j++){
				if(window[j].seqNum == ackSeqNum){
					window[j].seqNum = -1;
					window[j].armed = 0;
					window[j].resend = 0;
				}
			}
		}
		
		memset(ackBuf, 0, BUFLEN);
		i++;
	}

	close(fd);
    	fclose(fp);
	return 0;
}
