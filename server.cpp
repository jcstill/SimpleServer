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

int clientsocket = 0;
int sock = 0;

void sigintHandler(int sig_num){
	cout << "\nServer Terminating...\n";
	close(clientsocket);
	close(sock);
	exit(1);
} 

int main(){
	signal(SIGINT, sigintHandler);

	cout << "Creating Socket\n";
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		cout << "Error creating socket!\n";
		return 0;
	}

	cout << "Binding socket to ip and port\n";
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);
	if(bind(sock, (sockaddr*)&hint, sizeof(hint)) == -1){
		cout << "Error binding socket to port!\n";
		return 0;
	}

	cout << "Marking socket for listening on\n";
	if(listen(sock, SOMAXCONN) == -1){
		cout << "Error listening on socket!\n";
		return 0;
	}

	cout << "Waiting for connection\n";
	sockaddr_in client;
	socklen_t clientsize = sizeof(client);
	char host[NI_MAXHOST] = {0};
	char svc[NI_MAXSERV] = {0};
	clientsocket = accept(sock, (sockaddr*)&client, &clientsize);
	cout << "Accepting connection\n";
	if(clientsocket == -1){
		cout << "Error with client connecting!\n";
		return 0;
	}

	cout << "Close listening socket\n";
	close(sock);
	int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);
	if(result){
		cout << host << " connected on " << svc << '\n';
	}else{
		inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
		cout << host << " connected on " << ntohs(client.sin_port) << '\n';
	}


	int pid = fork();
	if(pid == 0){
		// Listening Process
		cout << "Waiting for message\n";
		char buffer[4096] = {0};
		while(true){
			// Clear buffer
			memset(buffer, 0, 4096);
			// Wait for message
			int bytesrecv = recv(clientsocket, buffer, 4096, 0);
			if(bytesrecv == -1){
				cout << "Error with connection!\n";
				break;
			}
			if(bytesrecv == 0){
				cout << "Client disconnected!\n";
				break;
			}
			// Display message
			cout << "Received: " << string(buffer, 0, bytesrecv) << '\n';
		}
	}else if(pid > 0){
		// Sending Process
		string dataSend;
		char buffer[4096] = {0};
		while(true){
			// Clear buffer
			memset(buffer, 0, 4096);
			// Get user Input
			getline(cin, dataSend);
			// Send to client
			int sendRes = send(clientsocket, dataSend.c_str(), dataSend.size()+1, 0);
			if(sendRes == -1){
				cout << "Error sending to server!\n";
				continue;
			}
		}
	}else{
		// fork failed
		printf("fork() failed!\n");
		return 1;
	}
	kill(pid, SIGTERM);

	// Close socket
	close(clientsocket);

	return 0;
}