# Client-Server-Architecture


## Overview

Using socket programming API in C, I have created a server capable of establishing connection with multiple clients and then implemented
Stop-and-Wait based data link layer level logical channel between server and client to provide error detection functionality for the recieved 
message using Cyclic Redundancy Code.
This was done as a part of Computer Networks Lab assignment in semester 6 @ IIT Guwahati under CSE dept with 2 other team members.



## Theoretical Background

Socket programming is used to form a connection between two nodes, one acts as a server which listens to the request made by the other node known as client.
 
### Server Socket Creation
This involves 4 steps, 
* Socket Creation
socket creation using socket(domain, type, protocol) api, which takes 3 arguments and returns an int (socket descriptor) for refering the created socket. If returned value is 0 means the socket creation process failed.
1) int: communication domain, AF_INET for IPV4 protocol and AF_INET6 for IPV6 protocol
2) int: SOCK_STREAM for TCP type connection and SOCK_DGRAM for UDP connection
3) int: protocol, which is 0 for IP

* Setsockopt
This step is an optional step which could be used to manipulate the options referred by socket descriptor. Can be used to allow server to  have multiple parallel client connection. It can also be used to allow address reuse.


* Bind
` int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);`
bind helps to bind the socket to the address provided in the second argument we bind the server to the localhost, hence we use INADDR_ANY to specify the IP address.                      


* Listen
` int listen(int sockfd, int backlog);`
Listen allows to set the server to listen for incoming request from client, the backlog argument is used to define the maximum length upto which pending connection can get queued up.


* Accept
` int new_socket= accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);`

It extracts the first connection request on the queue of pending connections for the listening socket, sockfd, creates a new connected socket, and returns a new file descriptor referring to that socket. At this point, connection is established between client and server, and they are ready to transfer data.



## Code Details


### server.c



### client.c

