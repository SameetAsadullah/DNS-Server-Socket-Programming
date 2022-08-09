#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<vector>
#include<pthread.h>
using namespace std;

#define MAXFD 10	//Size of fds array

struct clientsInfo{		//routing table
	int clientPort, serverPort, fd; 
	bool available;

	clientsInfo() {
		available = false;
	}
};
vector<clientsInfo> clients;

int portNumber;		//globally declaring portNumber to use in a thread

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

void* receiveFromProxyDNS(void* arg) { //receives from proxyDNS then decodes the message then does the work accordingly
	int sockfd1 = *((int*)arg), clientPort, serverPort;

	while(1) {
		char buff[500];
		int res=recv(sockfd1, buff, 500, 0);
		buff[res] = '\0';

		if (buff[0] == '1') { //if new client has connected with any server
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
			cout << "Client with port number " << clientPort;
			cout << " has connected with server of port " << serverPort << "." << endl;
		}

		else if (buff[0] == '2') {	//if any client has disconnected from any server
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
			cout << "Client with port number " << clientPort; 
			cout << " has disconnected from server of port " << serverPort << "." << endl;
		}

		else if (strncmp(buff, "Available.", 10) == 0) {
			int clientPort;

			//tokenizing and storing the message
			char *token = strtok(buff, "|");
			token = strtok(NULL, "\0");
			clientPort = stoi(token);
		
			for (int j = 0; j < clients.size(); ++j) {
				if (clients[j].clientPort == clientPort) {
					clients[j].available = true;
					break;
				}
			}
		}

		else if (buff[0] == '4') {	//sending the IP to the client
			string msgS = "";
			char *msgC;
			int clientPort;

			for (int j = 0; buff[j] != '\0'; ++j) {
				msgS += buff[j];
			}

			if (portNumber == 6000) {
				msgS += "|Server1";
			}
			else {
				msgS += "|Server2";
			}
			stringToChar(msgS, msgC);
			
			strtok(buff, "|");
			strtok(NULL, "|");
			strtok(NULL, "|");
			clientPort = stoi(strtok(NULL, "|"));

			for (int j = 0; j < clients.size(); ++j) {
				if (clients[j].clientPort == clientPort) {
					send(clients[j].fd, msgC, msgS.size(), 0);
					break;
				}
			}
		}

		else {	//if it's a message to pass to the client
			char *msg;

			//tokenizing and storing the message
			char *token = strtok(buff, "|");
			token = strtok(NULL, "|");
			msg = token;
			token = strtok(NULL, "\0");
			clientPort = stoi(token);
			
			//sending message to the client
			for (int i = 0; i < clients.size(); ++i) {
				if (clients[i].clientPort == clientPort) {
					send(clients[i].fd, msg, 500, 0);
					break;
				}
			}
		}
	}
}

int main()
{
	int ports[2] = {6000, 7000};
	int res = -1;
	pthread_t thread_id;
	bool connection_open = true;

	//socket for connection with clients
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd!=-1);

	struct sockaddr_in saddr,caddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family=AF_INET;
	saddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	for (int i = 0; (i < 2) && (res == -1); ++i) {
		saddr.sin_port=htons(ports[i]);
		res=bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
		if (res != -1) {
			cout << "\t\t\tSERVER ON PORT " << ports[i] << " IS UP NOW" << endl << endl;
			portNumber = ports[i];
		}
	}
	assert(res!=-1);
	
	//Create listening queue
	listen(sockfd,5);
	
   	//Define fdset collection
    	fd_set fdset;

	//defining fds array
	int fds[MAXFD];
    	for( int i = 0; i < MAXFD; ++i)
    	{
	  	fds[i]=-1;
    	}
	
	//Add a file descriptor to the fds array
    	fds_add(fds,sockfd);

	//socket for connection with proxyDNS server
	int sockfd1 = socket(AF_INET,SOCK_STREAM,0);	
	assert(sockfd1 != -1 );

	//Set Address Information
	struct sockaddr_in saddr1;
	memset(&saddr1,0,sizeof(saddr1));
	saddr1.sin_family = AF_INET;
	saddr1.sin_port = htons(9000);
	saddr1.sin_addr.s_addr = inet_addr("127.0.0.1");

	//Link to server
	int res1 = connect(sockfd1,(struct sockaddr*)&saddr1,sizeof(saddr1));
	assert(res1 != -1);

	//handshaking with proxyDNS
	string msgS = "Hello ProxyDNS," + to_string(portNumber);
	char *msgC;
	stringToChar(msgS, msgC);
	send(sockfd1, msgC, msgS.size(), 0);
	msgS.clear();
	delete[] msgC;

	//creating thread for receiving from proxyDNS
	pthread_create(&thread_id, NULL, receiveFromProxyDNS, (void*)&sockfd1);

	while(1)
    	{
		FD_ZERO(&fdset); //Clear the fdset array to 0
		int maxfd=-1;

		//For loop finds the maximum subscript for the ready event in the fds array
		for( int i = 0; i < MAXFD; i++)
		{
			if ( fds[i] == -1 )
			{
				continue;
			}

			FD_SET(fds[i],&fdset);
			if ( fds[i] > maxfd )
			{
				maxfd=fds[i];
			}
		}

		struct timeval tv={5,0};	//Set timeout of 5 seconds 
		int n = select(maxfd+1,&fdset,NULL,NULL,&tv); //Select system call, where we only focus on read events
		if(n==-1) //Fail
		{
			perror("select error");
		}
		else if(n==0) //Timeout, meaning no file descriptor returned
		{
			//do nothing
		}
		else { //Ready event generation
		//Because we only know the number of ready events by the return value of select, we don't know which events are ready.
		//Therefore, each file descriptor needs to be traversed for judgment
			for(int i = 0; i < MAXFD; ++i)
			{
				if(fds[i]==-1)	//If fds[i]==-1, the event is not ready
				{
					continue;
				}
				if(FD_ISSET(fds[i],&fdset))	//Determine if the event corresponding to the file descriptor is ready
				{
				//There are two kinds of cases for judging file descriptors
			   
					if(fds[i]==sockfd)	
					//A file descriptor is a socket, meaning accept if a new client requests a connection
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

						//storing client information in routing table
						cout << "Client with port number " << ntohs(caddr.sin_port) << " has connected to this server." << endl;
						clientsInfo client;
						client.clientPort = ntohs(caddr.sin_port);
						client.serverPort = ntohs(saddr.sin_port);
						client.fd = c;
						clients.push_back(client);

						//sending new client information to proxyDNS server
						msgS = "1|" + to_string(client.clientPort) + "|" + to_string(portNumber);
						stringToChar(msgS, msgC);
						send(sockfd1, msgC, msgS.size(), 0);
						msgS.clear();
						delete[] msgC;
					}
					else   //Receive data recv when an existing client sends data
					{
						char buff[500]={0};
						int res=recv(fds[i], buff, 500, 0);
						buff[res] = '\0';
						if(res<=0)
						{
							int clientPort;
							//deleting client information of this server
							for (int j = 0; j < clients.size(); ++j) {
								if (clients[j].fd == fds[i]) {
									clientPort = clients[j].clientPort;
									clients.erase(clients.begin() + j);		
									break;
								}
							}
							close(fds[i]);
							fds[i] = -1;

							cout << "Client with port number " << clientPort << " has disconnected from this server." << endl;

							//sending deleted client information to proxyDNS server
							msgS = "2|" + to_string(clientPort);
							stringToChar(msgS, msgC);
							send(sockfd1, msgC, msgS.size(), 0);
							msgS.clear();
							delete[] msgC;
						}
						else
						{
							if (strcmp(buff, "Available.") == 0) {
								msgS = "Available.|";
								for (int j = 0; j < clients.size(); ++j) {
									if (clients[j].fd == fds[i]) {
										clients[j].available = true;
										msgS += to_string(clients[j].clientPort);
										break;
									}
								}

								//letting proxy server know that the client is available for communicating
								stringToChar(msgS, msgC);
								send(sockfd1, msgC, msgS.size(), 0);
								msgS.clear();
								delete[] msgC;
							}

							else if (strcmp(buff, "Hello Server.") == 0) {
								string message = "";
								int index = -1;

								for (int j = 0; j < clients.size(); ++j) {	//getting index of the vector for port number of the connected client
									if (clients[j].fd == fds[i]) {
										index = j;
										break;
									}
								}

								for (int j = 0; j < clients.size(); ++j) {
									if (clients.size() != 1 && clients[j].available == true) {
										if (j == index) {
											message = message + "Y" + to_string(clients[j].clientPort);
										}

										else {
											message += to_string(clients[j].clientPort);
										}
										message += ',';
									}
								}

								if (message == "") {
									message = "No Client.";
								}
								char* temp;
								stringToChar(message, temp);
								send(fds[i], temp, message.size(), 0);
								delete[] temp;
							}

							else if (buff[0] == 'w' && buff[1] == 'w' && buff[2] == 'w' && buff[3] == '.' && (buff[res - 1] == 'm' || buff[res - 1] == 'k')) {
								//encoding message to send
								msgS = "4|";
								for (int j = 0; buff[j] != '\0'; ++j) {
									msgS += buff[j];
								}
								msgS += '|';
								for (int j = 0; j < clients.size(); ++j) {
									if (clients[j].fd == fds[i]) {
										msgS += to_string(clients[j].clientPort);
										break;
									}
								}
								//sending
								stringToChar(msgS, msgC);
								send(sockfd1, msgC, msgS.size(), 0);
								msgS.clear();
								delete[] msgC;
							}

							else {
								int cPort;
								string str_msg = "";
								char *message;
								char *token = strtok(buff, "|");
								message = token;
								str_msg += token;
								
								while (token) {
									token = strtok(NULL, "\0");
									if (token != NULL) {
										cPort = stoi(token);
									}
								}
								
								for (int j = 0; j < clients.size(); ++j) {
									if (clients[j].clientPort == cPort) {
										if (clients[j].serverPort == portNumber) {
											if (connection_open == true) {
												send(clients[j].fd, message, 500, 0);	//Send message to client
											}
											else {
												msgS = "";
												msgS += "Connection unavailable";
												stringToChar(msgS, message);
												send(fds[i], message, msgS.size(), 0);
												delete[] message;
												msgS.clear();
											}
											break;
										}

										else {
											int clientPort;
											for (int j = 0; j < clients.size(); ++j) {
												if (clients[j].fd == fds[i]) {
													clientPort = clients[j].clientPort;		
													break;
												}
											}
											
											if (connection_open == true) {
												connection_open = false;
											}
											if (connection_open == false && str_msg == "close") {
												connection_open = true;
											}
										
											msgS = "3|";
											msgS = msgS + message + "|" + to_string(cPort); 
											msgS += "|" + to_string(clientPort);
											stringToChar(msgS, message);
											send(sockfd1, message, msgS.size(), 0);	//Passing message to proxyDNS
											delete[] message;
											msgS.clear();
											break;
										}	
									}

									if (j == clients.size() - 1) {
										msgS = "close";
										stringToChar(msgS, message);
										send(fds[i], message, msgS.size(), 0);	//Passing message to proxyDNS
										msgS.clear();
										delete[] message;
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
