#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>          //strlen  
#include <stdlib.h>  
#include <unistd.h>         //close  
#include <sys/time.h>       //FD_SET, FD_ISSET, FD_ZERO macros 
#include <arpa/inet.h>      //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
 

#define MAX 10000     
#define TRUE   1  
#define FALSE  0  
#define CHAR_BIT 8
int close_soc = 0;

//Function declaration
 
void sigintHandler(int sig_num);
char* stringToBinary(char *s );
unsigned long binaryToDecimal(char *binary, int length);
void binaryToText(char *binary, int binaryLength, char *text, int symbolCount);
char* binaryToString(char *binary);
char* message_gen(char *msg);
int crc_check(char *msg);
char* error_gen(char *s, double p);
     
int main(int argc , char *argv[])   
{ 
    int master_socket;					//socket file descriptor for main socket
    int addrlen;						//address length 
    int new_socket;						
    int client_socket[10];				//list of clients (stores client socket fd)
    int max_clients = 10;
    int activity;						// return value of select
    int valread;						// length of the read buffer is something is read
    int sd;   							//socket descriptor
    int max_sd;							//max socket descriptor passed as parameter in select
    int opt = TRUE;
    char buffer[MAX];  					//data buffer of 1K  
    struct sockaddr_in serv_address;	//server address structure	
	double prob = 0;						//error probability
	printf("Enter the Error Probability : ");
	scanf("%lf", &prob);  

    fd_set readfds;						// Set of Socket Descriptors 
        
    //Initializes all Client Sockets to 0  
    for (int i = 0; i < max_clients; i++)   
    {   
        client_socket[i] = 0;   
    }   
         
    //Creates a Master Socket if successful.
    if( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)   
    {   
        perror("Socket Creation Failed\n");   
        exit(EXIT_FAILURE);   
    }
    else
    {
    	printf("Socket Created Successfully\n");
    }   
     
 
    
    //"setsockopt" allows Master Socket to connect to Multiple parallel tcp connections
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        perror("Multiple Connection Failed\n");   
        exit(EXIT_FAILURE);   
    }   
    else
    {
    	printf("Multiple Connection Allowed\n");
    }
	
	//Empties the structure "serv_address"
    bzero(&serv_address, sizeof(serv_address)); 

    //type of socket created  
    serv_address.sin_family = AF_INET;   			
    serv_address.sin_addr.s_addr = INADDR_ANY;   
    serv_address.sin_port = htons(atoi(argv[1]));   
         
    //bind the socket to provided host
    if (bind(master_socket, (struct sockaddr *)&serv_address, sizeof(serv_address)) < 0)   
    {   
        perror("Socket Binding Failed\n");   
        exit(EXIT_FAILURE);   
    }   
    else
    {
    	printf("Socket Binding Successful\n");
    }   
          
    if (listen(master_socket, 5) < 0)   //maximum pending client request
    {   
        perror("Listen Failed\n");   
        exit(EXIT_FAILURE);   
    }   
    else
    {
    	printf("Server Listening\n");
    }
         
    signal(SIGINT, sigintHandler);			//to catch CTRL+C
    addrlen = sizeof(serv_address);   
    
    while(TRUE)   
    {  
        //Makes the Socket Set empty 
        FD_ZERO(&readfds);   

        //Master Socket added to FD set  
        FD_SET(master_socket, &readfds);   
        max_sd = master_socket;  //Socket file Descriptor 
         
        //Adds valid Socket file descriptors to the fd sets
        for ( int i = 0 ; i < max_clients ; i++)   
        {   
		    sd = client_socket[i];  
		         
		    //if valid socket descriptor then add to read list  
		    if(sd > 0)   
		        FD_SET(sd, &readfds);   
		         
		    //stores the maximum file descriptor for select function  
		    if(sd > max_sd)   
		        max_sd = sd;   
        }   

       	// Waits indefinitely for an activity from any of the client side     
        activity = select( max_sd + 1, &readfds, NULL, NULL, NULL);

		// If server is closed, it gracefully closes all the connected client socket connections
        if(close_soc){
            for(int i=0; i<max_clients; i++){
        	   sd = client_socket[i];
        	   if(sd > 0){
        		  getpeername(sd, (struct sockaddr*)&serv_address, (socklen_t*)&addrlen);   
        		  printf("Disconnecting Client : IP %s and port %d \n", inet_ntoa(serv_address.sin_addr), ntohs(serv_address.sin_port));     
        		  close( sd );   
        		  client_socket[i] = 0;           		
        	   }
            }
            break;
        }
		
		// Checks for error in any of the connections
        if ((activity < 0) && (errno != EINTR)) {   
            printf("Error in one of the connections\n");   
        }   
         
        // Check if master socket is in the readfds set of client fd
        if (FD_ISSET(master_socket, &readfds)) {   
        	// accepts incoming requests from clients and rejects if error
            if ((new_socket = accept(master_socket, (struct sockaddr *)&serv_address, (socklen_t*)&addrlen)) <0 ){   
                perror("Error in Accepting Request\n ");   
                exit(EXIT_FAILURE);   
            }   

            //displays new socket connection
            printf("New connection : Socket FD is %d, IP is %s, Port is %d\n", new_socket, inet_ntoa(serv_address.sin_addr), ntohs(serv_address.sin_port));   
              
            //add new socket to array of sockets  
            for (int i = 0; i < max_clients; i++) {   
                //if position is empty  
                if( client_socket[i] == 0 ) {   
                    client_socket[i] = new_socket;     
                    break;   
                }   
            }   
        }   
         
        //checks for actvity in each client
        for (int i = 0; i < max_clients; i++){   
        	//extracts socket descriptor of the currently serving client
            sd = client_socket[i];    
            //check if its present in socket descriptor set
            if (FD_ISSET(sd, &readfds)){   
                //Check if it was for closing , and also read the  
                //incoming message  ,
                if ((valread = read(sd, buffer, MAX)) == 0){   
                    //Displays the detail of client that disconnected  
                    getpeername(sd, (struct sockaddr*)&serv_address, (socklen_t*)&addrlen);   
                    printf("Client Disconnected : IP is %s , Port is %d \n", inet_ntoa(serv_address.sin_addr), ntohs(serv_address.sin_port));   
                         
                    //Free the socket for future use  
                    close( sd );   
                    client_socket[i] = 0;   
                }   
             
                else {   
                    //terminate the string
                    buffer[valread] = '\0'; 
                    //checking for error in the read message    
            		if(crc_check(buffer)){
        		  		getpeername(sd, (struct sockaddr*)&serv_address, (socklen_t*)&addrlen);           		  		            		
                        buffer[strlen(buffer) - 8] = '\0';
    					strcpy(buffer, binaryToString(buffer));				//convert read message to binary 
                        printf("Message Received : %s from IP %s and port %d \n", buffer, inet_ntoa(serv_address.sin_addr), ntohs(serv_address.sin_port));           
    					printf("Sending ACK\n");
    					char * ACK = stringToBinary("ACK");
    					bzero(buffer, MAX); 
    					strcpy(buffer, ACK);					    //copy ACK on buffer
    					strcpy(buffer, message_gen(buffer));		//Add CRC bits
    					strcpy(buffer, error_gen(buffer,prob));		//Generate error
    					send(sd, buffer, sizeof(buffer),0);			//send Acknowledgement
    					bzero(buffer, MAX);							//empty the buffer

            		}

            	    else{
                    	getpeername(sd, (struct sockaddr*) &serv_address, (socklen_t*) &addrlen); 
                    	printf("Packets Received in Error from IP %s and port %d \n", inet_ntoa(serv_address.sin_addr), ntohs(serv_address.sin_port));
                    	printf("Sending NACK\n");
    					char* NACK = stringToBinary("NACK");
    					bzero(buffer, MAX); 
    					strcpy(buffer, NACK);
    					strcpy(buffer, message_gen(buffer));
    					strcpy(buffer, error_gen(buffer, prob));
    					send(sd, buffer, sizeof(buffer), 0);
    					bzero(buffer, MAX);

            		}
                } //ending else  
            }  //ending if 
        }	//ending for  
    }   //ending while
    printf("\nClosing Server Socket\n");
    close(master_socket);     
    return 0;   
} 

/* Below Signal Handler is called when "Ctrl+C" is pressed by the user, it sets the global var "close_soc" to 1.
It initiates the closing of master socket and closing all the client sockets linked to it. */
void sigintHandler(int sig_num) 
{
    close_soc = 1;
} 

/* Following func converts a string to its binary form */ 
char *stringToBinary(char *s)
{
    char *ptr;
    size_t slen  = strlen(s);
    char *binary = malloc(slen * CHAR_BIT + 1);
    char *start  = binary;
    errno = 0;

    if (s == NULL){
        return NULL;
    }

    if(binary == NULL){
        fprintf(stderr, "Failed To allocate space in function stringToBinary(%s): %s\n", s, strerror(errno));
        return NULL;
    }

    if (slen == 0){
        *binary = '\0';
        return binary;
    }
    for (ptr = s; *ptr != '\0'; ptr++){
        /* perform bitwise AND for every bit of the character */
        // loop over the input-character bits
        for (int i = CHAR_BIT - 1; i >= 0; i--, binary++){
            *binary = (*ptr & 1 << i) ? '1' : '0';
        }
    }
    *binary = '\0';
    binary = start;    // reset pointer to beginning
    return binary;
}

/*  helper function for string to binary*/
void binaryToText(char *binary, int binaryLength, char *text, int symbolCount){
    int i;
    for(i = 0; i < binaryLength; i+=8, binary += 8){
        char *byte = binary;
        byte[8] = '\0';
        *text++ = binaryToDecimal(byte, 8);
    }
    text -= symbolCount;
}

/*  helper function used in binary to tex*/
unsigned long binaryToDecimal(char *binary, int length){
    int i;
    unsigned long decimal = 0;
    unsigned long weight = 1;
    binary += length - 1;
    weight = 1;
    for(i = 0; i < length; ++i, --binary){
        if(*binary == '1')
            decimal += weight;
        weight *= 2;
    }
    
    return decimal;
}

// Converts the binary message received from the client and returns the corresponding string
char* binaryToString(char *binary)
{
    int binaryLength = strlen(binary);
    int symbolCount = binaryLength / 8 + 1;
    char *text = malloc(symbolCount + 1);
    if(text == NULL || binary == NULL)
        exit(1);
    if(binaryLength % 8 == 0)
        --symbolCount;
        
    binaryToText(binary, strlen(binary), text, symbolCount);
    return text;
}

// Calculates CRC of the original message and appends it to the end of the original message and returns it
char* message_gen(char* msg)
{
    int m = 0;
    int n = 0;
    char gen[] = "100000111";
    char temp[10000];
    strcpy(temp, msg);				                    //temp---> copy of original message        
    while(msg[m] != '\0'){
        m++;
    }
    while(gen[n] != '\0'){
        n++;
    }
   
    for(int i=0; i<n-1; i++){                           //Appending 8 '0s' at the end of message
        temp[m+i] = '0';
    }
    temp[m+n-1] = '\0';
    msg[m] = '\0';   
    m = m+n-1;  
    for(int i=0; i <= m-n; i++){
        if(temp[i] == '1'){			                    //division by generator polynomial
            for(int j=0; j<n; j++){                     //bitwise XOR					
                if(temp[i+j] != gen[j]){
                    temp[i+j] = '1';
                }
                else if(temp[i+j] == gen[j] && gen[j] == '1'){
                    temp[i+j] = '0';
                }
            }
            while(i<m-n-1 && temp[i+1] != '1'){
                i++;
            }               
        }
    }
    char ans[10000];
    for(int i=0; i<n-1; i++){					       // ans--> last 8 bits of the CRC generated
        ans[i] = temp[m-n+1+i];
    }
    ans[n-1] = '\0';   					                //termnating string
    strcat(msg, ans); 					                //appending CRC to original message
    return msg;
}


/* Chack if the T(x) received from client is divisible by the CRC-8 generator polynomial 
    Retruns 0 if error exists else returns 1*/
int crc_check(char* msg){
    char gen[] = "100000111";
    char temp[10000];
    strcpy(temp, msg);
    int m = 0;
    int n = 0;     
    while(msg[m] != '\0'){                                  // calculating length
        m++;
    }
    while(gen[n] != '\0'){
        n++;
    }
    msg[m] = '\0';   
    for(int i=0; i <= m-n; i++){
        if(temp[i] == '1'){				                    //Performing division by bitwise XOR
            for(int j=0; j<n; j++){
                if(temp[i+j] != gen[j]){			        //bit=1 if different bits 
                    temp[i+j] = '1';
                }
                else if(temp[i+j] == gen[j] && gen[j] == '1'){
                    temp[i+j] = '0';
                }
            }
            while(i<m-n-1 && temp[i+1]!='1'){			    //traversing to find the first '1'
                i++;
            } 
        }
    }
    for(int i=0; i<m; i++){						            //if any bit is 1, report error
        if(temp[i] == '1'){
            return 0;
        }
    }
    return 1;								                //all bits zero return 1 ie no error
}


/* Introducing error in one random bit of the T(x) according to user given probability */
char *error_gen(char *s, double p)		//code to generate error
{
	int lower = 0;						
	int upper = strlen(s)-1;
	double random_num = (double)rand() / (double)RAND_MAX;
	printf("Random Number generated  : %lf\n",random_num);
	if(random_num <= p){
											//to generate random number in range [0, messageLength-1]
		int index = rand()%(upper-lower+1) + lower;
		if(s[index] == '1'){				//bit flip at index calculated randomly
			s[index] = '0';
		}
		else{
			s[index] = '1';
		}
	}
	return s;							//return message containing error
}
