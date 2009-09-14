#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <locale.h>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "tcp.h"

void sendline(int sock, const char* str, const char* val) {
	int len = strlen(str);
	char* line;
	if (val) {
		len += strlen(val);
		line = (char*) malloc(len + 3);
		strcpy(line, str);
		strcat(line, val);
		strcat(line, "\r\n");
	} else {
		line = (char*) malloc(len + 3);
		strcpy(line, str);
		strcat(line, "\r\n");
	}
	send(sock, line, len + 2, 0);
	free(line);
}

char* recvline(int sock) {
	const int growsize = 80;
	char c = 0;
	char* line = (char*) malloc(growsize);
	int len = growsize, pos = 0;
	while (line) {
		if (recv(sock, &c, 1, 0) <= 0) break;
		if (c == '\r') continue;
		if (c == '\n') break;
		line[pos++] = c;
		if (pos >= len) {
			len += growsize;
			line = (char*) realloc(line, len);
		}
	}
	line[pos] = 0;
	return line;
}

int create_socket(const char* server) {
	int sock = -1;
	struct sockaddr_in serv_addr;
	struct hostent* host_ent;
	char* host = strdup(server);
	char* port = strchr(host, ':');

	if (port) *port++ = 0;
	else port = "23053";

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		sock = -1;
		goto leave;
	}

	if ((host_ent = gethostbyname(host)) == NULL) {
		perror("gethostbyaddr");
		sock = -1;
		goto leave;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr, host_ent->h_addr, host_ent->h_length );
	serv_addr.sin_port = htons((short)atol(port));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		sock = -1;
		goto leave;
	}
leave:
	if (host) free(host);
	return sock;
}

void close_socket(int sock) {
#ifdef _WIN32
	if (sock < 0) closesocket(sock);
#else
	if (sock < 0) close(sock);
#endif
}
