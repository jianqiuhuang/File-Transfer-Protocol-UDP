AUTHORS:
Ellington Kiby and Jianqiu Huang 

This is a small program that demonstrates basic communication using UDP sockets with Selective Repeat Protocol

PORT NUMBER is fixed in port.h

TO RUN:
    //client
    ./udp-send <file-path> <file-name> <server-IP>

    //server
    ./udp-recv <file-path>

ACKNOWLEDGEMENT:

    During the coding process, we drew much of the implementation for the socket setup and sending in UDP from  Paul Krzyzanowski (Rutger University) at https://www.cs.rutgers.edu/~pxk/417/notes/sockets/files/demo-udp-04.zip.

PACKET FORMAT:
----------------------------------------------------------------------------------------------------
| 508 Bytes for the data potion | 4 bytes for an unsigned integer representing the sequence number|
----------------------------------------------------------------------------------------------------
We chose a window size of 16. 

Both server and client must agree to the same packet format.
