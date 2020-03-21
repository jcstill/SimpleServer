#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <string>
#include <signal.h>
using namespace std;
#define PORT 54000

int clientsocketc2p = 0;
int sockc2p = 0;
int sockp2s = 0;

void sigintHandler(int sig_num){
	cout << "\nProxy Terminating...\n";
	close(clientsocketc2p);
	close(sockc2p);
	close(sockp2s);
} 

int main(int argc, char *argv[]){
	string ipaddress = "\0";
	if(argc >= 2){
		if(argv[1] != "\0"){
			ipaddress = argv[1];
		}
	}
//--------------------------------------------------------------------
//Setup Client to Proxy Side
//--------------------------------------------------------------------
	signal(SIGINT, sigintHandler);
	cout << "INFO c2p: Creating socket\n";
	sockc2p = socket(AF_INET, SOCK_STREAM, 0);
	if(sockc2p == -1){
		cout << "ERRORc2p: Error creating socket!\n";
		sigintHandler(1);
		return 0;
	}
	cout << "INFO c2p: Binding socket to ip and port\n";
	sockaddr_in hintc2p;
	hintc2p.sin_family = AF_INET;
	hintc2p.sin_port = htons(PORT);
	inet_pton(AF_INET, "0.0.0.0", &hintc2p.sin_addr);
	if(bind(sockc2p, (sockaddr*)&hintc2p, sizeof(hintc2p)) == -1){
		cout << "ERRORc2p: Error binding socket to port!\n";
		sigintHandler(1);
		return 0;
	}
	cout << "INFO c2p: Marking socket for listening on\n";
	if(listen(sockc2p, SOMAXCONN) == -1){
		cout << "ERRORc2p: Error listening on socket!\n";
		sigintHandler(1);
		return 0;
	}
	cout << "INFO c2p: Waiting for connection\n";
	sockaddr_in client;
	socklen_t clientsize = sizeof(client);
	char host[NI_MAXHOST] = {0};
	char svc[NI_MAXSERV] = {0};
	clientsocketc2p = accept(sockc2p, (sockaddr*)&client, &clientsize);
	cout << "INFO c2p: Accepting connection\n";
	if(clientsocketc2p == -1){
		cout << "ERRORc2p: Error with connecting!\n";
		sigintHandler(1);
		return 0;
	}
	cout << "INFO c2p: Close listening socket\n";
	close(sockc2p);
	int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);
	if(result){
		cout << "INFO c2p: " << host << " connected on " << svc << '\n';
	}else{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		cout << "INFO c2p: " << host << " connected on " << ntohs(client.sin_port) << '\n';
	}
//--------------------------------------------------------------------
//Ask Client for Server info
//--------------------------------------------------------------------
	string dataSend = "Proxy: Enter Server IP Address: ";
	if(ipaddress == "\0"){
		char buffer[4096] = {0};
		// Send to Client
		int sendRes = send(clientsocketc2p, dataSend.c_str(), dataSend.size()+1, 0);
		if(sendRes == -1){
			cout << "ERRORc2p: Error sending to client!\n";
			sigintHandler(1);
			return 0;
		}
		// Wait for message
		int bytesrecv = recv(clientsocketc2p, buffer, 4096, 0);
		if(bytesrecv == -1){
			cout << "ERRORc2p: Error with connection!\n";
			sigintHandler(1);
			return 0;
		}
		if(bytesrecv == 0){
			cout << "ERRORc2p: Client disconnected!\n";
			sigintHandler(1);
			return 0;
		}
		// Display message
		ipaddress = string(buffer, 0, bytesrecv);
		cout << "INFO p2s: Connecting to: " << ipaddress << '\n';
	}
//--------------------------------------------------------------------
//Setup Proxy to Server Side
//--------------------------------------------------------------------
	cout << "INFO p2s: Creating socket\n";
	sockp2s = socket(AF_INET, SOCK_STREAM, 0);
	if(sockp2s == -1){
		cout << "ERRORp2s: Error creating socket!\n";
		sigintHandler(1);
		return 0;
	}
	sockaddr_in hintp2s;
	hintp2s.sin_family = AF_INET;
	hintp2s.sin_port = htons(PORT);
	inet_pton(AF_INET, ipaddress.c_str(), &hintp2s.sin_addr);
	cout << "INFO p2s: Connecting to the server on the socket\n";
	int connectRes = connect(sockp2s, (sockaddr*)&hintp2s, sizeof(hintp2s));
	if(connectRes == -1){
		cout << "ERRORp2s: Error connecting to server!\n";
		sigintHandler(1);
		return 0;
	}
	cout << "INFO p2s: Connected to server!\n";
	dataSend = "Proxy: Connected to Srever!";
	int sendRes = send(clientsocketc2p, dataSend.c_str(), dataSend.size()+1, 0);
	if(sendRes == -1){
		cout << "ERRORc2p: Error sending to client!\n";
		sigintHandler(1);
		return 0;
	}
//--------------------------------------------------------------------
	int pid = fork();
	if(pid == 0){
		// Sending Process
		char buffer[4096] = {0};
		while(true){
			// Clear buffer
			memset(buffer, 0, 4096);
			// Wait for message
			int bytesrecvc2p = recv(clientsocketc2p, buffer, 4096, 0);
			if(bytesrecvc2p == -1){
				cout << "ERRORc2p: Error with connection!\n";
				sigintHandler(1);
				return 0;
			}
			if(bytesrecvc2p == 0){
				cout << "ERRORc2p: Client disconnected!\n";
				kill(pid, SIGTERM);
				sigintHandler(1);
				return 0;
			}
			// Display message from client
			cout << "DATA c2p: " << string(buffer, 0, bytesrecvc2p) << '\n';
			// Send message to server
			int sendRes = send(sockp2s, buffer, bytesrecvc2p+1, 0);
			if(sendRes == -1){
				cout << "ERROR p2s: Error sending to server!\n";
				continue;
			}
		}
	}else if(pid > 0){
		// Listening Process
		char buffer[4096] = {0};
		while(true){
			// Clear buffer
			memset(buffer, 0, 4096);
			// Wait for response from server
			int bytesrecvp2s = recv(sockp2s, buffer, 4096, 0);
			if(bytesrecvp2s == -1){
				cout << "ERRORs2p: Error receiving from server!\n";
				return 0;
			}else if(buffer[0] == '\0'){
				cout << "ERRORs2p: Error receiving from server!\n";
				return 0;
			}else{
				// Display message from server
				cout << "DATA p2s: " << string(buffer, 0, bytesrecvp2s) << '\n';
				// Send message to client
				int sendRes = send(clientsocketc2p, buffer, bytesrecvp2s+1, 0);
				if(sendRes == -1){
					cout << "ERROR p2s: Error sending to server!\n";
					continue;
				}
			}
		}
	}else{
		// fork failed
		printf("fork() failed!\n");
	}
	kill(pid, SIGTERM);
	// Close socket
	close(clientsocketc2p);
	close(sockp2s);
	return 0;
}
