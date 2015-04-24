#include <iostream> 
#include <stdlib.h>	
#include <string.h> 
#include <sstream>
#include <set>

#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "database.h"

using namespace std;

set<int> clients;

bool read_socket(int socketfd, string &line);
bool write_to_socket(int socketfd, string line);
void* player(void* args);
void broadcast(int originfd, string text);

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, port = 9999;
	socklen_t client_addr_length;
	struct sockaddr_in serv_addr, cli_addr;
	string line;

	initDB();
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		 cout << "Sorry, error creating socket!" << endl;
		 exit(-1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);

	int res = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	if (res < 0) {
		 cout << "Sorry, error binding to socket" << endl;
		 exit(-1);
	}

	listen(sockfd, 5);

	while (true) {
		client_addr_length = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &client_addr_length);

		pthread_t thread;
		pthread_create(&thread, NULL, player, &newsockfd);
	}
	
	close(sockfd);
	
	closeDB();
	
	return 0; 
}

bool read_socket(int socketfd, string &line) {
	int n; 
	char buffer[1025]; 
	
	line = "";

	while (line.find('\n') == string::npos) {
		int n = read(socketfd, buffer, 1024);
		if (n == 0) return false;
		buffer[n] = 0; 
		line += buffer;
	}
	
	line.erase(line.end() - 1);
	line.erase(line.end() - 1);
	return true;  
}

bool write_to_socket(int socketfd, string line) {
	string tosend = line + "\n";
	write(socketfd, tosend.c_str(), tosend.length());
	
	return true;
}

void* player(void* args) {
	int sockfd = *(int*)args;
	string line, command, arg1, arg2;
	
	clients.insert(sockfd);
	cout << "Reading from socket " << sockfd << endl;
	while (read_socket(sockfd, line)) {
		cout << "Socket " << sockfd << " said: " << line << endl;
		
		istringstream iss(line);
			
		iss >> command >> arg1 >> arg2;
		
		PGresult* res = executeSQL("SELECT * FROM users WHERE uid = '" + arg1 + "' AND pass = '" + arg2 + "'");
		for (int row = 0; row < PQntuples(res); row++)
			cout << PQgetvalue(res, row, 0) << ' ' << PQgetvalue(res, row, 1) << endl;
	}

	cout << "Closing socket " << sockfd << endl;
	
	clients.erase(sockfd);

	close(sockfd);
	
	return NULL;
}

void broadcast (int origin, string text) {
	ostringstream message;
	message << origin << " said: " << text;

	set<int>::iterator it;
	for (it = clients.begin(); it != clients.end(); it++)
	  if (*it != origin) write_to_socket(*it, message.str());
}