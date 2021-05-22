# Client-Server-Architecture


## Overview

Using socket programming API in C, I have created a server capable of establishing connection with multiple clients and then implemented
Stop-and-Wait based data link layer level logical channel between server and client to provide error detection functionality for the recieved 
message using Cyclic Redundancy Code.
This was done as a part of Computer Networks Lab assignment in semester 6 @ IIT Guwahati under CSE dept with 2 other team members.



## Theoretical Background

Socket programming is used to form a connection between two nodes, one acts as a server which listens to the request made by the other node known as client.
 
### Socket Creation
This involves 4 steps, 
socket creation using socket(domain, type, protocol) api, which takes 3 arguments and returns an int (socket descriptor) for refering the created socket. If returned value is 0 means the socket creation process failed.
1) int: communication domain, AF_INET for IPV4 protocol and AF_INET6 for IPV6 protocol
2) int: SOCK_STREAM for TCP type connection and SOCK_DGRAM for UDP connection
3) int: protocol, which is 0 for IP



## Code Details


### server.c



### client.c

