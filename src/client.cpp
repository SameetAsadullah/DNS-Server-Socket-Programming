#include<iostream>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<vector>
#include<algorithm>
using namespace std;

void stringToChar(string buff, char *&temp) {	//converting string into char*
	temp = new char[buff.size() + 1];
	buff.copy(temp, buff.size() + 1);
	temp[buff.size()] = '\0';
}

int main()
{
	int portNumber, myPort = 0;
	int number = -1;
	bool check = true, getClients = true, check1 = true, check2 = false;

	//menu for servers
	cout << "1) Server with port 6000." << endl << "2) Server with port 7000." << endl << "3) Server with port 8000." << endl;
	while (portNumber != 6000 && portNumber != 7000 && portNumber != 8000) {
		cout << "Enter port number on which you want to connect: ";
		cin >> portNumber;
	}

	int sockfd = socket(AF_INET,SOCK_STREAM,0);	
	assert(sockfd != -1 );

	//Set Address Information
	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(portNumber);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	//Link to server
	int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
	assert(res != -1);

	//displaying menu and getting IP of website
	int menu;
	cout << endl << "1) Communication with client." << endl << "2) Get IP of a website." << endl;
	while (menu != 1 && menu != 2) {
		cout << "What type of connection do you want?(1/2) ";
		cin >> menu;

		if (menu == 2) {	//getting IP
			string msgS;
			char *msgC;
			int siteNo;
			vector<char*> path;

			while (siteNo != 1 && siteNo != 2 && siteNo != 3 && siteNo != 4 && siteNo != 5) {	//asking available websites
				cout << endl << "1) Google." << endl << "2) Amazon." << endl << "3) Daraz." << endl << "4) Stackoverflow." << endl << "5) Coursera." << endl;
				cout << "Enter number of website whose ip you want to get: ";
				cin >> siteNo;
			}

			if (siteNo == 1) {
				msgS = "www.google.com";
			}

			else if (siteNo == 2) {
				msgS = "www.amazon.com";
			}

			else if (siteNo == 3) {
				msgS = "www.daraz.pk";
			}

			else if (siteNo == 4) {
				msgS = "www.stackoverflow.com";
			}

			else {
				msgS = "www.coursera.com";
			}

			//sending
			stringToChar(msgS, msgC);
			send(sockfd, msgC, msgS.size(), 0);
			msgS.clear();
			delete[]msgC;

			//receiving
			msgC = new char[500];
			int n = recv(sockfd, msgC, 500, 0);
			msgC[n] = '\0';

			//displaying IP and Path
			char* token;

			if (portNumber == 8000) {
				token = strtok(msgC, "|");
				cout << endl << "IP Received: " << token << endl;
				strtok(NULL, "|");
			}
			else {
				strtok(msgC, "|");
				token = strtok(NULL, "|");
				cout << endl << "IP Received: " << token << endl;
				strtok(NULL, "|");
				strtok(NULL, "|");
			}

			while (token) {
				token = strtok(NULL, "|\0");
				if (token) {
					path.push_back(token);
				}
			}

			cout << "Path: ";
			for (int j = 0; j < path.size(); ++j) {
				cout << path[j];
				cout << " -> ";
				
			}
			cout << "You" << endl;

			delete[] msgC;
			close(sockfd);
			exit(0);
		}
	}

	//letting server know that this client is available for communication
	char msg[11] = "Available.";
	send(sockfd, msg, sizeof(msg), 0);

	//communication with client
	while(1)
	{
		vector<int> clients;
		string message;
		char *buff;

		if (getClients == true) {
			//Handshaking and getting clients ports
			message = "Hello Server.";
			stringToChar(message, buff);
			sleep(1);
			send(sockfd, buff, message.size(), 0);
			delete[] buff;
			buff = new char[500];
			int n = recv(sockfd, buff, 500, 0);
			buff[n] = '\0';
	
			//if there's no client for communication
			if (strcmp(buff, "No Client.") == 0) {
				if (check == true && strcmp(buff, "close") != 0 && check2 == false) {
					cout << endl << "NO OTHER CLIENT IS AVAILABLE FOR COMMUNICATION YET." << endl;
					check = false;
					check1 = false;
				}
				delete[] buff;
				continue;
			}

			//storing and displaying menu for clients
			char* temp;
			char *token = strtok(buff, ",");
			while (token) {
				if (token[0] != 'Y') {
					clients.push_back(stoi(token));
				}
				else {
					temp = token;
				}
				token = strtok(NULL, ",");
			}

			if (myPort == 0) {
				string msgS = "";
				for (int j = 1; temp[j] != '\0'; ++j) {
					msgS += temp[j];
				}
				myPort = stoi(msgS);
			}

			if (clients.empty()) {
				if (check1 == true) {
					cout << endl << "NO OTHER CLIENT IS AVAILABLE FOR COMMUNICATION YET." << endl;	
					check1 = false;
					check2 = true;
				}
				continue;
			}

			cout << endl << "-> Your Port: " << myPort << endl << "-> Connected with server: " << portNumber << endl << endl;
			for (int i = 0; i < clients.size(); ++i) {
				cout << i + 1 << ") Client with port no " << clients[i] << "." << endl; 
			}	

			number = -1;
			while(find(clients.begin(), clients.end(), number) == clients.end() && number != 0) {
				cout << "Enter port number(0 for search again) on which you want to connect: ";
				cin >> number;
			}
			if (number == 0) {
				continue;
			}

			cin.ignore();
			getClients = false;
			delete[] buff;
		}

		//sending message
		string copy_msg;
		printf("\nEnter Message: ");
		getline(cin, message);
		copy_msg = message;
		message = message + "|" + to_string(number);
		stringToChar(message, buff);
		send(sockfd, buff, message.size(), 0);
		delete[] buff;
		
		//receiving message
		buff = new char[500];
		int n = recv(sockfd, buff, 500, 0);
		buff[n] = '\0';
		printf("Message Received: %s\n", buff);
	

		//closing connection
		if(copy_msg == "close" || strcmp(buff, "close") == 0 || strcmp(buff, "Connection unavailable") == 0) {
			char option;

			while(option != 'n' && option != 'N' && option != 'y' && option != 'Y') {
				cout << endl << "Do you want to communicate with any other client?(y/n) ";
				cin >> option;
			}

			if (option == 'Y' || option == 'y') {
				check = true;
				getClients = true;
				check1 = true;
			}

			else {
				break;
			}
		}

		if (copy_msg != "close") {
			delete[] buff;
		}
	}

	close(sockfd);
}