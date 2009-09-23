#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "tcp.h"


void growl_tcp_write( int sock , const char *const format , ... ) 
{
	va_list ap;

	va_start( ap , format );
	int length = vsnprintf( NULL , 0 , format , ap );
	va_end(ap);

	va_start(ap,format);
	char *output = malloc(length+1);
	vsnprintf( output , length+1 , format , ap );
	va_end(ap);

	send( sock , output , length , 0 );
	send( sock , "\r\n" , 2 , 0 );

	free(output);
}

char *growl_tcp_read(int sock) {
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

int growl_tcp_open(const char* server) {
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

void growl_tcp_close(int sock) {
#ifdef _WIN32
	if (sock < 0) closesocket(sock);
#else
	if (sock < 0) close(sock);
#endif
}
