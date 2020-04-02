#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret, k = 0;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char topic[TOPIC_LEN + 1], *aux;


	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;
	if (argc < 4) {
		usage(argv[0]);
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	int flags = 1;
	if (setsockopt(sockfd, SOL_TCP, TCP_NODELAY, (void *)&flags,
	               sizeof(flags))) {
		perror("ERROR: setsocketopt(), TCP_NODELAY");
		exit(0);
	}

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	n = send(sockfd, argv[1], strlen(argv[1]), 0);
	DIE(n < 0, "send");

	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	int cam_open = 0;
	while (1) {

		while (cam_open == 1) {
			FILE *in = fopen("connected.txt", "rt");
			if (in) {
				char name[100];
				while (fgets(name, 100, in)) {
					printf("%s", name);
					if (strcmp(name, "Vlad\n") == 0) {
						send(sockfd, "unlock\n", 8, 0);
						cam_open = 0;
						fclose(in);

						system("pkill python");
						system("rm connected.txt");
						break;
					}
				}
			} else {
				printf("!in\n");
			}
			usleep(1000000);


		}

		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL); // masca biti
		DIE(ret < 0, "select");


		if (FD_ISSET(sockfd, &tmp_fds)) {
			// s-a primit mesaj de la server
      		memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			DIE(n < 0, "recv");
			if (n == 0) {
				// conexiune inchisa
				break;
			}
			if (buffer[0] != 0) {
				printf("%s", buffer);

				/* SCREEN IS LOCKED */
				if (!cam_open && strncmp(buffer, "lock", 4) == 0) {
					printf("Display locked!!\n");
					system("rm connected.txt 2> /dev/null");
					system("python 03_face_recognition.py &");
					usleep(1000000);
					cam_open = 1;
				}
			} else {
				printf("Disconnecting...\n");
				break;
			}
		}
	}

	close(sockfd);

	return 0;
}
