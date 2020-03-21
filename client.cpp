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
// #define IP "192.168.1.30"

int main(int argc, char *argv[]){
	string ipaddress;
	if(argc >= 2){
		if(argv[1] != "\0"){
			ipaddress = argv[1];
		}
	}else{
		cout << "enter IP to connect to: ";
		cin >> ipaddress;
		cin.ignore(10000, '\n');
	}
	cout << "Creating Socket\n";
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1){
		cout << "Error creating socket!\n";
		return 0;
	}

	// Create a hint structure for the server we're connecting with
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(PORT);
	inet_pton(AF_INET, ipaddress.c_str(), &hint.sin_addr);

	cout << "Connecting to the server on the socket\n";
	int connectRes = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if(connectRes == -1){
		cout << "Error connecting to server!\n";
		return 0;
	}
	cout << "Connected to Server!\n";
	string dataSend;
	int pid = fork();
	if(pid == 0){
		// Sending Process
		do{
			// Get user Input
			getline(cin, dataSend);
			// Send to server
			int sendRes = send(sock, dataSend.c_str(), dataSend.size()+1, 0);
			if(sendRes == -1){
				cout << "Error sending to server!\n";
				continue;
			}
		}while(true);
	}else if(pid > 0){
		// Listening Process
		char buffer[4096] = {0};
		do{
			// Clear buffer
			memset(buffer, 0, 4096);
			// Wait for response
			int bytesrecv = recv(sock, buffer, 4096, 0);
			if(bytesrecv == -1){
				cout << "Error receiving from server!\n";
				break;
			}else if(buffer[0] == '\0'){
				cout << "Error receiving from server!\n";
				break;
			}else{
				// Display response
				cout << "Received: " << string(buffer, 0, bytesrecv) << '\n';
			}
		}while(true);
	}else{
		// fork failed
		printf("fork() failed!\n");
	}
	kill(pid, SIGTERM);
	// Close the socket
	close(sock);
	return 0;
}
