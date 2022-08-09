#include<vector>
#include<fstream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<iostream>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
using namespace std;
#define MAXFD 10	//Size of fds array

void fds_add(int fds[],int fd)	//Add a file descriptor to the fds array
{
	for (int i = 0; i < MAXFD; ++i)
	{
		if (fds[i] == -1)
		{
	      		fds[i]=fd;
		  	break;
		}
	}
}

void stringToChar(string buff, char *&temp) {	//converting string into char*
	temp = new char[buff.size() + 1];
	buff.copy(temp, buff.size() + 1);
	temp[buff.size()] = '\0';
}

struct domain_against_IP {
	string domain_name;
	string IP;
};

int main()
{
	vector <domain_against_IP> vec_name_IP;
	fstream IP_file("IPs.txt", ios::in);
	if (IP_file.is_open()) {
		while(!IP_file.eof()) {
			domain_against_IP obj_name_IP;
			IP_file >> obj_name_IP.domain_name;
			IP_file >> obj_name_IP.IP;
			vec_name_IP.push_back(obj_name_IP);
		}
	}

	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd != -1);
    
	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(5000);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int client_count = 0;
	int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);
	
	// Create listening queue
	listen(sockfd,1);
	
	// Define fdset collection
	fd_set fdset;
	
	// Define fds array
    	int fds[MAXFD];
    	for(int i = 0; i < MAXFD; ++i) {
	  	fds[i] = -1;
    	}
	
	// Add a file descriptor to the fds array
    	fds_add(fds,sockfd);

	while(1)
    	{
		FD_ZERO(&fdset);// Clear the fdset array to 0

		int maxfd = -1;
		int i = 0;

		// For loop finds the maximum subscript for the ready event in the fds array
		for (; i < MAXFD; i++)
		{
			if (fds[i] == -1)
			{
				continue;
			}

			FD_SET(fds[i], &fdset);

			if (fds[i] > maxfd)
			{
				maxfd = fds[i];
			}
		}

		struct timeval tv = {5,0};	// Set timeout of 5 seconds

		int n = select(maxfd+1, &fdset, NULL, NULL, &tv);	// Selectect system call, where we only focus on read events
		if ( n == -1 )	// Fail
		{
			perror("select error");
		}
		else if ( n == 0 )	// Timeout, meaning no file descriptor returned
		{
			//printf("time out\n");
		}
		else {	
		// Ready event generation
		// Because we only know the number of ready events by the return value of select, we don't know which events are ready.
		// Therefore, each file descriptor needs to be traversed for judgment
			
			for (i = 0; i < MAXFD; ++i) {
				if (fds[i] == -1) {	// If fds[i]==-1, the event is not ready
					continue;
				}
				if (FD_ISSET(fds[i], &fdset)) {	
					// Determine if the event corresponding to the file descriptor is
					// There are two kinds of cases for judging file descriptors
			   
					if (fds[i] == sockfd) {	
						// A file descriptor is a socket, meaning accept if a new client requests a connection
						// Accept
						struct sockaddr_in caddr;
						socklen_t len = sizeof(caddr);

						int c = accept(sockfd,(struct sockaddr *)&caddr,&len);	// Accept new client connections
						if(c < 0) {
							continue;
						}
						
						client_count++;
						fds_add(fds,c);
						// Add the connection socket to the array where the file descriptor is stored
					}
					
					else { // Receive data recv when an existing client sends data
						char buff[500] = {0};
						int res = recv(fds[i],buff,500,0);
						buff[res] = '\0';
						if (res <= 0) { // Client has exited/closed connection
							close(fds[i]);
							fds[i] = -1;
							client_count--;
						}
						else if (buff[0] != '\0'){ 
							int count = 0;
							string tosend = "", tosend1 = "", clport = "";
							for (int i = 0; buff[i] != '\0'; i++) {
								if (buff[i] == '|')
									count++;
								else if (count == 0)
									tosend1 += buff[i];
								else if (count == 1) 
									tosend += buff[i];
								else if (count == 2)
									clport += buff[i];
							}
							
							if (count == 0) {
								char *temp;
								stringToChar(tosend1,temp);
								printf("Received to obtain IP for: %s\n", temp);
								delete[] temp;
								
								count = 0;
								for (int i = 0; i < vec_name_IP.size(); i++) {
									if (tosend1 == vec_name_IP[i].domain_name) {
										count++;
										tosend = "";
										tosend += vec_name_IP[i].IP;
										tosend += "|" + vec_name_IP[i].domain_name + "|DNS";
										break;
									}
								}
								
								if (count == 0) {
									string name = tosend1;
									tosend += "IP not found...";
									tosend += "|" + name;
									tosend += "|DNS";
								}
								
								stringToChar(tosend,temp);
								send(fds[i],temp,tosend.size(),0);
								delete[]temp;
							}
							else {
								char *temp;
								stringToChar(tosend,temp);
								printf("Received to obtain IP for: %s\n", temp);
								delete[] temp;
								
								count = 0;
								for (int i = 0; i < vec_name_IP.size(); i++) {
									if (tosend == vec_name_IP[i].domain_name) {
										count++;
										tosend = "4|";
										tosend += vec_name_IP[i].IP;
										tosend += "|" + vec_name_IP[i].domain_name;
										tosend += "|" + clport + "|" + "DNS";
										break;
									}
								}
								
								if (count == 0) {
									string name = tosend;
									tosend = "4|";
									tosend += "IP not found...";
									tosend += "|" + name;
									tosend += "|" + clport + "|" + "DNS";
								}
								
								stringToChar(tosend,temp);
								send(fds[i],temp,tosend.size(),0);
								delete[]temp;
							}
						}
					}				
				}
			}
		}
	}
}
