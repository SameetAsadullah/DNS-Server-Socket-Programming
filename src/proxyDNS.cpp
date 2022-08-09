#include<iostream>
#include<vector>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
using namespace std;

#define MAXFD 10	//Size of fds array

struct clientsInfo {	//routingTable
	int clientPort, serverPort;
	bool available;

	clientsInfo() {
		available = false;
	}
};

struct serversInfo {	//stores information about servers
	int /*serverPort, */listeningPort, fd;

	serversInfo() {
		//serverPort = 0;
		fd = 0;
	}
};

void fds_add(int fds[],int fd)	//Add a file descriptor to the fds array
{
	int i=0;
	for(;i<MAXFD;++i)
	{
		if(fds[i]==-1)
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

int main()
{
	vector<clientsInfo> clients;	//routing table
	serversInfo servers[3];	//stores information about servers

	string siteName = "", IP = "";

	int sockfd=socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);
	
	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(9000);
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");

	int res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res!=-1);
	
	//Create listening queue
	listen(sockfd,5);
	
	//Define fdset collection
	fd_set fdset;
	
	//Define fds array
    	int fds[MAXFD];
    	int i=0;
    	for(;i<MAXFD;++i)
    	{
	  	fds[i]=-1;
    	}
	
	//Add a file descriptor to the fds array
    	fds_add(fds,sockfd);
    	
    	// variables to check if connection is busy
	int c1 = -1, c2 = -1, close_count = 0;
	bool connection_open = true;

	while(1)
    	{
		FD_ZERO(&fdset);//Clear the fdset array to 0

		int maxfd=-1;

		int i=0;

		//For loop finds the maximum subscript for the ready event in the fds array
		for(;i<MAXFD;i++)
		{
			if(fds[i]==-1)
			{
				continue;
			}

			FD_SET(fds[i],&fdset);

			if(fds[i]>maxfd)
			{
				maxfd=fds[i];
			}
		}

		struct timeval tv={5,0};	//Set timeout of 5 seconds

		int n=select(maxfd+1,&fdset,NULL,NULL,&tv);//Selectect system call, where we only focus on read events
		if(n==-1)	//fail
		{
			perror("select error");
		}
		else if(n==0)//Timeout, meaning no file descriptor returned
		{
			//do nothing
		}
		else//Ready event generation
		{
		//Because we only know the number of ready events by the return value of select, we don't know which events are ready.
		//Therefore, each file descriptor needs to be traversed for judgment
			for(i=0;i<MAXFD;++i)
			{
				if(fds[i]==-1)	//If fds[i]==-1, the event is not ready
				{
					continue;
				}
				if(FD_ISSET(fds[i],&fdset))	//Determine if the event corresponding to the file descriptor is ready
				{
			   
				//There are two kinds of cases for judging file descriptors
			   
					if(fds[i]==sockfd)	//A file descriptor is a socket, meaning accept if a new client requests a connection
					{
						//accept
						struct sockaddr_in caddr;
						socklen_t len=sizeof(caddr);

						int c=accept(sockfd,(struct sockaddr *)&caddr,&len);	//Accept new client connections
						if(c<0)
						{
							continue;
						}
						fds_add(fds,c);//Add the connection socket to the array where the file descriptor is stored

						//storing connected servers information
						for (int j = 0; j < 3; ++j) {
							if (servers[j]./*serverPort*/fd == 0) {
								//servers[j].serverPort = ntohs(caddr.sin_port);
								servers[j].fd = c;
								break;
							}
						}
					}
					else   //Receive data recv when an existing client sends data
					{
						char buff[500]={0};
						int res=recv(fds[i],buff,500,0);
						buff[res] = '\0';

						if(res<=0)
						{
							close(fds[i]);
							fds[i]=-1;
						}
						else
						{
							if (strcmp(buff, "Hello ProxyDNS,6000") == 0) {		//storing information if server of port 6000 is handshaking
								for (int j = 0; j < 3; ++j) {
									if (servers[j].fd == fds[i]) {
										servers[j].listeningPort = 6000;
										break;
									}
								}
							}							

							else if (strcmp(buff, "Hello ProxyDNS,7000") == 0) {	//storing information if server of port 7000 is handshaking
								for (int j = 0; j < 3; ++j) {
									if (servers[j].fd == fds[i]) {
										servers[j].listeningPort = 7000;
										break;
									}
								}
							}

							else if (strcmp(buff, "Hello ProxyDNS,8000") == 0) {	//storing information if server of port 8000 is handshaking
								for (int j = 0; j < 3; ++j) {
									if (servers[j].fd == fds[i]) {
										servers[j].listeningPort = 8000;
										break;
									}
								}
							}

							else {	//other message
								//copying the message to send later on
								char msg[500];
								for (int j = 0; buff[j] != '\0'; ++j) {
									msg[j] = buff[j];
								}
								msg[res] = '\0';
								
								if (buff[0] == '1') {	//if new client has connected with any server
									int clientPort, serverPort;

									//tokenizing and storing the message
									char *token = strtok(buff, "|");
									token = strtok(NULL, "|");
									clientPort = stoi(token);
									token = strtok(NULL, "\0");
									serverPort = stoi(token);
									
									//adding new client in routing table
									clientsInfo client;
									client.clientPort = clientPort;
									client.serverPort = serverPort;
									clients.push_back(client);

									//displaying new client on terminal
									cout << "Client with port number " << clientPort << " has connected with server of port " << serverPort << "." << endl;

									//sending information of new client to the servers
									for (int j = 0; j < 3; ++j) {
										if (servers[j].listeningPort != serverPort) {
											send(servers[j].fd,msg,sizeof(buff),0);
										}
									}
								}

								else if (buff[0] == '2') {	//if any client has disconnected from any server
									int clientPort, serverPort;

									//tokenizing and storing the message
									char *token = strtok(buff, "|");
									token = strtok(NULL, "\0");
									clientPort = stoi(token);
									
									//removing client from routing table
									for (int i = 0; i < clients.size(); ++i) {
										if (clients[i].clientPort == clientPort) {
											serverPort = clients[i].serverPort;
											clients.erase(clients.begin() + i);
											break;
										}
									}

									//displaying new client on terminal
									cout << "Client with port number " << clientPort << " has disconnected from server of port " << serverPort << "." << endl;

									//sending information of disconnected client to the servers
									for (int j = 0; j < 3; ++j) {
										if (servers[j].listeningPort != serverPort) {
											send(servers[j].fd,msg,sizeof(buff),0);
										}
									}
								}

								else if (strncmp(buff, "Available.", 10) == 0) {
									int clientPort, serverPort;

									//tokenizing and storing the message
									char *token = strtok(buff, "|");
									token = strtok(NULL, "\0");
									clientPort = stoi(token);
								
									for (int j = 0; j < clients.size(); ++j) {
										if (clients[j].clientPort == clientPort) {
											clients[j].available = true;
											serverPort = clients[j].serverPort;
											break;
										}
									}

									//letting other servers know about the availability
									for (int j = 0; j < 3; ++j) {
										if (servers[j].listeningPort != serverPort) {
											send(servers[j].fd, msg, sizeof(buff), 0);
										}
									}
								}

								else if (buff[0] == '4') {		//if any client has asked for IP
									if (buff[2] == 'w' && buff[3] == 'w' && buff[4] == 'w' && buff[5] == '.') {		//getting the IP
										strtok(buff, "|");
										if (strtok(NULL, "|") == siteName) {	//if proxyDNS has the IP stored in cache
											char* msgC;
											string msgS = "4|";
											msgS = msgS + IP + "|" + siteName + "|" + strtok(NULL, "\0") + "|ProxyDNS";
											stringToChar(msgS, msgC);
											send(fds[i], msgC, msgS.size(), 0);
											delete[] msgC;
										}

										else {		//else getting IP from the DNS server
											for (int j = 0; j < 3; ++j) {
												if (servers[j].listeningPort == 8000) {
													send(servers[j].fd, msg, sizeof(msg), 0);
													break;
												}
											}
										}
									}

									else {		//sending back the IP
										string msgS = "";
										char *msgC;
										int clientPort;

										for (int j = 0; buff[j] != '\0'; ++j) {
											msgS += buff[j];
										}
										msgS += "|ProxyDNS";
										stringToChar(msgS, msgC);
									
										siteName.clear();
										IP.clear();

										strtok(buff, "|");		
										IP += strtok(NULL, "|");
										siteName += strtok(NULL, "|");
										clientPort = stoi(strtok(NULL, "|"));

										for (int j = 0; j < clients.size(); ++j) {
											if (clients[j].clientPort == clientPort) {
												for (int k =0; k < 3; ++k) {
													if (servers[k].listeningPort == clients[j].serverPort) {
														send(servers[k].fd, msgC, msgS.size(), 0);
														delete[] msgC;
														k = 3;
														j = clients.size();
													}
												}
											}
										}
									}
								}

								else {	//if it's a message to pass to the client
									int clientPort, serverPort, source_cport;
									char* message;
									string str_message = "";

									//tokenizing and storing the message
									char *token = strtok(buff, "|");
									token = strtok(NULL, "|");
									message = token;
									str_message += token;
									token = strtok(NULL, "|"); // storing destination client port
									clientPort = stoi(token);
									token = strtok(NULL, "\0"); // storing source client port
									source_cport = stoi(token);
									
									//check if connection is busy
									if ((c1 == -1 && c2 == -1) || (connection_open == false && (c2 == source_cport || c1 == source_cport))) {
									// for a new connection
										connection_open = false;
										c1 = clientPort;
										c2 = source_cport;
										
										string str_msg = "";
										str_msg = str_msg + "3|" + message + "|" + to_string(clientPort);
										
										int c = 0;
										char *tempmsg;
										stringToChar(str_msg,tempmsg);
										for (int j = 0; tempmsg[j] != '\0'; ++j) {
											msg[j] = tempmsg[j];
											c++;
										}
										msg[c] = '\0';
										delete []tempmsg;
										
										//getting server port
										for (int j = 0; j < clients.size(); ++j) {
											if (clients[j].clientPort == clientPort) {
												serverPort = clients[j].serverPort;
												break;
											}
										}
										
										if (str_message == "close") {
										// connection closing process
											close_count++;
											if (close_count == 2) {
												c1 = -1;
												c2 = -1;
												connection_open = true;
												close_count = 0;
											}
											
										}
									}
									else {
									// if connection is already established
										str_message = "";
										str_message += "3|Connection unavailable|" + to_string(source_cport);
										
										int c = 0;
										char *tempmsg;
										stringToChar(str_message,tempmsg);
										for (int j = 0; tempmsg[j] != '\0'; ++j) {
											msg[j] = tempmsg[j];
											c++;
										}
										msg[c] = '\0';
										delete []tempmsg;
										
										//getting server port
										for (int j = 0; j < clients.size(); ++j) {
											if (clients[j].clientPort == source_cport) {
												serverPort = clients[j].serverPort;
												break;
											}
										}
									}

									//sending message to the server
									for (int j = 0; j < 3; ++j) {
										if (servers[j].listeningPort == serverPort) {
											send(servers[j].fd,msg,sizeof(buff),0);
											break;
										}
									}					
								}
							}
						}
					}
				}
			}
		}
	}
}
