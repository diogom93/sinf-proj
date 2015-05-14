#include <iostream> 
#include <stdlib.h>	
#include <string.h> 
#include <sstream>

#include "command.h"

using namespace std;

pthread_mutex_t durex = PTHREAD_MUTEX_INITIALIZER;

ofstream log_file("log.txt", ios::app);

void* player(void* args);

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

void* player(void* args) {
	int sockfd = *(int*)args;
	string line;
	
	write_to_socket(sockfd, "\n                   WELCOME TO");
	char help_line[256];
	ifstream file_in("banner.txt");
	while (!file_in.eof()) {
		file_in.getline(help_line, 255);
		write_to_socket(sockfd, help_line);
	}
	
	log_file << "Reading from socket " << sockfd << endl;
	while (read_socket(sockfd, line)) {
		log_file << "Socket " << sockfd << " said: " << line << endl;
		
		pthread_mutex_lock(&durex);
		if (check_command(sockfd, line) == false) {
			break;
		}
		pthread_mutex_unlock(&durex);
	}
	pthread_mutex_unlock(&durex);
	log_file << "Closing socket " << sockfd << endl;

	close(sockfd);
	
	return NULL;
}