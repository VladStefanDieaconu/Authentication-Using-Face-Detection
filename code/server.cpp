#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <list>
#include <unordered_map>
#include <algorithm>
#include "helpers.h"

#define p std::make_pair

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

void initialise_connections(fd_set &read_fds, fd_set &tmp_fds,
                            int &sockfd, int portno, struct sockaddr_in
                            &serv_addr) {
	int ret;
	// se goleste multimea de descriptori de citire (read_fds)
	//    si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// connection socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni)
	// in multimea read_fds
	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
}

bool kill_everyone(char *buffer, int sockfd, char *conn) {
	int j, n;
	memset(buffer, 0, BUFLEN);
	fgets(buffer, BUFLEN - 1, stdin);
	if (strncmp(buffer, "exit", 4) == 0) {

		// send shutdown to all clients
		memset(buffer, 0, BUFLEN);
		for (j = sockfd + 1; j < MAX_CLIENTS + sockfd + 1; j++)
			if (conn[j] == 1) {
				n = send(j, buffer, BUFLEN, 0);
				DIE(n < 0, "send");
			}
		return true;
	}
	return false;
}

void accept_new_conn(char *message, char *buffer, int &fdmax, struct
                     sockaddr_in  &cli_addr, std::unordered_map<std::string,
                     std::list<std::pair<std::string, int>>> &topics,
                     char *conn,
                     std::unordered_map<std::string, std::list<
                     std::string>> &queued_topics, std::unordered_map<int,
                     std::string> &socket_to_id, int sockfd,
                     fd_set &read_fds, std::unordered_map<std::string, int>
                     &id_to_socket) {
	// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
	// pe care serverul o accepta
	int n, newsockfd;
	socklen_t clilen = sizeof(cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	DIE(newsockfd < 0, "accept");

	// se adauga noul socket intors de accept() la multimea descriptorilor de citire
	FD_SET(newsockfd, &read_fds);
	if (newsockfd > fdmax) {
		fdmax = newsockfd;
	}
	conn[newsockfd] = 1; // marcat ca deschis

	// primeste cloent_id
	memset(buffer, 0, BUFLEN);
	n = recv(newsockfd, buffer, sizeof(buffer), 0);
	DIE(n < 0, "recv");

	// il adaug map-ul de socketi
	socket_to_id.insert(std::make_pair(newsockfd, std::string(buffer)));
	id_to_socket.insert(std::make_pair(std::string(buffer), newsockfd));

	printf("New client %s connected from %s:%d\n", buffer,
	       inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

}


int main(int argc, char *argv[]) {
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	char message[BUFLEN];
	char conn[MAX_CLIENTS + 5];
	memset(conn, 0, MAX_CLIENTS + 5); // 0 for not connected
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, j, ret, sf;
	socklen_t clilen;

	// pentru fiecare client id avem un vector de topicuri si sf
	std::unordered_map<std::string,
	    std::list<std::pair<std::string, int>>> topics;
	std::unordered_map<std::string,
	    std::list<std::string>> queued_topics;
	std::unordered_map<int, std::string> socket_to_id;
	std::unordered_map<std::string, int> id_to_socket;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	initialise_connections(read_fds, tmp_fds, sockfd, portno,
	                       serv_addr);

	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL); // masca biti
		DIE(ret < 0, "select");

		if (FD_ISSET(0, &tmp_fds)) {
			if (kill_everyone(buffer, sockfd, conn))
				break;
			else {
				if (strncmp(buffer, "lock", 4) == 0) {
					printf("locking...\n");
					usleep(1000000);
					memset(buffer, 0, BUFLEN);
					sprintf(buffer, "%s\n", "lock");
					// send lock signal to all clients
					for (j = sockfd + 1; j < MAX_CLIENTS + sockfd + 1; j++)
						if (conn[j] == 1) {
							n = send(j, buffer, strlen(buffer), 0);
							DIE(n < 0, "send");
						}
					// system("gnome-screensaver-command -l");
				}
				continue;
			}
		}

		for (i = sockfd; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) { // check if set
				if (i == sockfd) {
					// socketul pe care se asteapta conexiuni tcp
					accept_new_conn(message, buffer, fdmax, cli_addr, topics, conn,
					                queued_topics, socket_to_id, sockfd, read_fds,
					                id_to_socket);
				} else {
					// se trateaza atat mesajele primite cat si deconectarea
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Client %s disconected\n", socket_to_id.at(i).c_str());
						close(i);
						conn[i] = 0;
						FD_CLR(i, &read_fds);
					} else {
						if (strncmp(buffer, "unlock", 6) == 0) {
							printf("unlocking...\n");
							system("gnome-screensaver-command -d");
							system("xdotool key BackSpace");
							continue;
						}
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
