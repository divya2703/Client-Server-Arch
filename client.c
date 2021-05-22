#include <math.h>
#include <time.h>
#include <errno.h>
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <arpa/inet.h>      //close  
#include <sys/types.h> 
#include <string.h> 
#include <limits.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>  

#define MAX 10000
#define SA struct sockaddr 

int close_soc=0; //flag set to 1 if client socket is closed

//Function Declarations

void sigintHandler(int sig_num);
char *stringToBinary(char *s);
char *message_gen(char *msg);
int crc_check(char *msg); 
char *error_gen(char *s,double p);

int main(int argc,char **argv) 
{ 
  	srand(time(0));          //Providing seed to generate random number
	
	double prob=0;			// prob stores probability of error
	int sock_num;  			//socket number
    int ready_for_reading;
    int reading;	
    int n;
	char buffer[MAX]; 				// buffer space where read message will be kept
    struct sockaddr_in serv_addr;	//s_addr---> server address
    struct sockaddr_in cli_addr;	//c_addr----> connection address
    struct timeval time_out;	
	
	printf("Enter the Error Probability : ");
    scanf("%lf", &prob);
    getchar();  	
	
	fd_set readfds;					// used to read input from multiple client

    time_out.tv_sec = 10;    		// timeout time = 10 seconds
    time_out.tv_usec = 0;	
	
    
    // CREATING SOCKET
    sock_num= socket(AF_INET, SOCK_STREAM, 0);  //AF_INET-->IPV4, SOCK_STREAM---> TCP full duplex 
    											//sock_num stores non negative file descriptor of the created socket
    											//used for further operations                                       
    if (sock_num == -1)
    {
        printf("Socket Creation FAILED");
        exit(0);
    }
    else
    {
        printf("Socket Creation SUCCESSFUL \n");
    }
    bzero(&serv_addr, sizeof(serv_addr)); 		//initializes buffer to zero

    FD_ZERO(&readfds);							//initializes file descriptor set to zero
    FD_SET(sock_num,&readfds);					//sets the bit for the file descriptor sock_num in the file descriptor set readfds.

	// ASSIGN IP, PORT
    serv_addr.sin_family= AF_INET;				       //IPV protocol    
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);    //Assigning IP addr
    serv_addr.sin_port= htons(atoi(argv[2]));	       // Assigning port number
    
    
	// CONNECTING CLIENT SOCKET TO SERVER SOCKET
	int temp = connect(sock_num, (SA*)&serv_addr, sizeof(serv_addr));
	if (temp != 0)
	{ 
        printf("Connection with the Server Failed...\n"); 
        exit(0); 
    } 
    else
    {
        printf("Connected to the Server...\n"); 
	}
	
	signal(SIGINT, sigintHandler);      //catches CTRL+C (Interrupt)
	
	for(;;)								//to continue sending msg unless ctrl+c is pressed
	{
		clock_t start,end;
		double cpu_time_used;			
		bzero(buffer, sizeof(buffer)); 
		n = 0;							//iterator for reading char on input
		if(close_soc == 1)				//checking if there's interrupt
		{
			break;						//comes out of for loop
		}
		
		printf("Enter the string : ");  //taking input
		while (close_soc==0 && (buffer[n++] = getchar()) != '\n') ;   //continue reading message and storing it in
																	  // buffer until ctrl+c or enter is encountered
		buffer[n-1]='\0';											  // to mark the end of string
		if(close_soc == 1)
		{
			break;
		}
		 
		char org_msg[MAX];						
		
		strcpy(buffer,stringToBinary(buffer));						//conversion from string to binary
		strcpy(buffer, message_gen(buffer));						// generating T(x), message after adding CRC bits
		strcpy(org_msg,buffer);										// original message after adding CRC
		strcpy(buffer,error_gen(buffer,prob));						// adding error
		
		send(sock_num, buffer, sizeof(buffer),0); 					//sending server generated message
		bzero(buffer, sizeof(buffer)); 
		
		while(1)
		{
			FD_ZERO(&readfds);									//initializing fd_set readfds to empty
			FD_SET(sock_num, &readfds);							// assigning readfds socket file descriptor
			
			start = clock();									//starting clock
			ready_for_reading=select(sizeof(readfds)*8 ,&readfds,NULL,NULL,&time_out);  //checking if there's anything to be read until timeout
			end = clock();										
			
			if(ready_for_reading == 0)							//if there's nothing received after timeout
			{
				printf("TimeOut, Retransmitting Data\n");		
				bzero(buffer,sizeof(buffer));
				strcpy(buffer,org_msg);
				strcpy(buffer,error_gen(buffer,prob));
				send(sock_num, buffer, sizeof(buffer),0); 
				
				bzero(buffer,sizeof(buffer));
				time_out.tv_sec = 10;    						// Initializing the timer again
				time_out.tv_usec = 0;
			}
			else if(ready_for_reading == 1)						//If something is received, read from the sock_num
			{
				bzero(buffer,sizeof(buffer));
				read(sock_num, buffer, sizeof(buffer));				
				
				if(crc_check(buffer))							//if crc check successful, check whether its ACK or NACK
				{
					char buff_temp[MAX];
					int buff_len = strlen(buffer);
					buffer[buff_len-8]='\0';					// to remove the last 8 bit added after adding CRC
					strcpy(buff_temp,stringToBinary("ACK"));	//storing binary value of 'ACK'
					
					if(strcmp(buff_temp,buffer) == 0)			// if string comparison is successful,ACK is received
					{
						printf("Received ACK\n");
						
						time_out.tv_sec = 10;
						time_out.tv_usec = 0;
						break;
					}
					else
					{	
						printf("Received NACK, Retransmitting Data\n");
						
						bzero(buffer,sizeof(buffer));
						strcpy(buffer,org_msg);
						strcpy(buffer,error_gen(buffer,prob));
						send(sock_num, buffer, sizeof(buffer),0); 
						
						bzero(buffer,sizeof(buffer));
						time_out.tv_sec = 10;    				// Initializing timer to 10 seconds
						time_out.tv_usec = 0;						
					}
				}
				else
				{												//if comparison fails, start the timer again 
																// continuing after the time already elapsed
					printf("Error in ACK or NACK, Waiting ... \n");
					
					cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
					double t_out = (double)time_out.tv_sec + (double)time_out.tv_usec*pow(10,-6) - cpu_time_used;
					int sec = t_out;
					double mili = t_out - (double)sec;
					time_out.tv_sec = sec;
					time_out.tv_usec = mili*pow(10,6);	
				}				
			}
			else
			{
				time_out.tv_sec = 10;
				time_out.tv_usec = 0;
				break;
			}
		}
	}
	
    if(close_soc == 0)
    {
    	printf("Server Disconnected, Closing Socket \n");
    }
    else{
    	printf("Closing the Socket \n");
    }
    close(sock_num); 
    
    return 0;
}

void sigintHandler(int sig_num) 
{  
    close_soc = 1; 
    fclose(stdin);
}

char *stringToBinary(char *s)
{
	size_t slen = strlen(s);
	char *binary = malloc(slen * CHAR_BIT + 1);
	char *ptr;
	char *start = binary;	
	errno = 0;
	if(s == NULL)
	{
		return NULL;
	}
	
	if(binary == NULL)
	{
		fprintf(stderr,"Failed To Allocate Space in function stringToBinary(%s): %s\n",s, strerror(errno));
		return NULL;
	}

	if (slen == 0)
	{
		*binary = '\0';
		return binary;
	}

	for (ptr = s; *ptr != '\0'; ptr++)
	{
		/* perform bitwise AND for every bit of the character */
		// loop over the input-character bits
		for (int i = CHAR_BIT - 1; i >= 0; i--, binary++) {
			*binary = (*ptr & 1 << i) ? '1' : '0';
		}
	}

	*binary = '\0';
	binary = start;	// reset pointer to beginning
	return binary;
}

char *message_gen(char *msg)
{
    int m=0;
    int n=0;
    char gen[] = "100000111";
    char temp[10000];
    char ans[10000];    
    strcpy(temp,msg);
     
    while(msg[m] != '\0')
    {
        m++;
    }
    while(gen[n] != '\0')
    {
        n++;
    }
    
    for(int i = 0;i < n-1;i++)
    {
        temp[m+i]='0';
    }
    
    temp[m+n-1]='\0';
    msg[m]='\0';   
    m=m+n-1;  
    
    for(int i=0;i <= m-n;i++)
    {
        if(temp[i] == '1')
        {
            for(int j=0;j<n;j++)
            {
                if(temp[i+j] != gen[j])
                {
                    temp[i+j]='1';
                }
                else if(temp[i+j]==gen[j] && gen[j]=='1')
                {
                    temp[i+j]='0';
                }
            }
            while(i<m-n-1 && temp[i+1]!='1')
            {
                i++;
            }               
        }
    }
    
    for(int i=0;i<n-1;i++)
    {
        ans[i] = temp[m-n+1+i];
    }
    
    ans[n-1]='\0';   
    strcat(msg,ans);   
    return msg;
}

int crc_check(char *msg)
{
    int m=0;
    int n=0; 
    char gen[] = "100000111";		//polynomial for CRC-8 x^8 + x^2 + x + 1
    char temp[10000];		
    strcpy(temp,msg);
    
    while(msg[m] != '\0')
    {
        m++;
    }
    while(gen[n] != '\0')
    {
        n++;
    }
    msg[m]='\0'; 

    for(int i=0;i <= m-n;i++)
    {
        if(temp[i] == '1')
        {
            for(int j=0;j<n;j++)
            {
                if(temp[i+j]!=gen[j])
                {
                    temp[i+j]='1';
                }
                else if(temp[i+j]==gen[j] && gen[j]=='1')
                {
                    temp[i+j]='0';
                }
            }
            while(i<m-n-1 && temp[i+1]!='1')
            {
                i++;
            } 
        }
    }
    
    for(int i=0;i<m;i++)
    {
        if(temp[i] == '1')
        {
            return 0;
        }
    }
    return 1;
} 
char *error_gen(char *s,double p)		//code to generate error
{
	int lower = 0;						
	int upper = strlen(s)-1;
	double random_num = (double)rand() / (double)RAND_MAX;
	if(random_num<=p)
	{
											//to generate random number in range [0, messageLength-1]
		int index = rand()%(upper-lower+1) + lower;
		if(s[index] == '1')					//bit flip at index calculated randomly
		{
			s[index] = '0';
		}
		else
		{
			s[index] = '1';
		}
	}
	return s;							//return message containing error
}
 
