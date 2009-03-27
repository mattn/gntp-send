/**
 * msvc: cl growlc.c
 * mingw32: gcc -o growlc.exe growlc.c -lws2_32
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <locale.h>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

static void sendline(int sock, const char* str, const char* val) {
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

static char* recvline(int sock) {
	const static int growsize = 80;
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

int create_socket() {
	int sock = -1;
	struct sockaddr_in serv_addr;
	struct hostent* host_ent;
	char value = 1; 

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		printf("%d\n", WSAGetLastError());
		return -1;
	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	if ((host_ent = gethostbyname("127.0.0.1")) == NULL) {
		perror("gethostbyaddr");
		return -1;
	}
	memset((char *)&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr, host_ent->h_addr, host_ent->h_length );
	serv_addr.sin_port = htons((short)23053);

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		return -1;
	}
	return sock;
}

static void close_socket(int sock) {
#ifdef _WIN32
	if (sock < 0) closesocket(sock);
#else
	if (sock < 0) close(sock);
#endif
}

static char* string_to_utf8_alloc(const char* str) {
#ifdef _WIN32
	unsigned int codepage;
	size_t in_len = strlen(str);
	wchar_t* wcsdata;
	char* mbsdata;
	size_t mbssize, wcssize;

	codepage = GetACP();
	wcssize = MultiByteToWideChar(codepage, 0, str, in_len,  NULL, 0);
	wcsdata = (wchar_t*) malloc((wcssize + 1) * sizeof(wchar_t));
	wcssize = MultiByteToWideChar(codepage, 0, str, in_len, wcsdata, wcssize + 1);
	wcsdata[wcssize] = 0;

	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, NULL, 0, NULL, NULL);
	mbsdata = (char*) malloc((mbssize + 1));
	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, mbsdata, mbssize, NULL, NULL);
	mbsdata[mbssize] = 0;
	free(wcsdata);
	return mbsdata;
#else
	return strdup(str);
#endif
}

int main(int argc, char* argv[]) {
	int sock = -1;
	char* title = NULL;
	char* message = NULL;
	char* icon = NULL;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) goto leave;
	setlocale(LC_CTYPE, "");
#endif

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "%s: title message [icon]", argv[0]);
		exit(1);
	}

	title = string_to_utf8_alloc(argv[1]);
	message = string_to_utf8_alloc(argv[2]);
	if (argc == 4) icon = string_to_utf8_alloc(argv[3]);

	sock = create_socket();
	if (sock == -1) goto leave;
	sendline(sock, "GNTP/1.0 REGISTER NONE", NULL);
	sendline(sock, "Application-Name: My Example", NULL);
	sendline(sock, "Notifications-Count: 1", NULL);
	sendline(sock, "", NULL);
	sendline(sock, "Notification-Name: My Notify", NULL);
	sendline(sock, "Notification-Display-Name: My Notify", NULL);
	sendline(sock, "Notification-Enabled: True", NULL);
	sendline(sock, "", NULL);
	while (1) {
		char* line = recvline(sock);
		int len = strlen(line);
		/* fprintf(stderr, "%s\n", line); */
		free(line);
		if (len == 0) break;
	}
	close_socket(sock);

	sock = create_socket();
	if (sock == -1) goto leave;
	sendline(sock, "GNTP/1.0 NOTIFY NONE", NULL);
	sendline(sock, "Application-Name: My Example", NULL);
	sendline(sock, "Notification-Name: My Notify", NULL);
	sendline(sock, "Notification-Title: ", title);
	sendline(sock, "Notification-Text: ", message);
	if (icon) sendline(sock, "Notification-Icon: ", icon);
	sendline(sock, "", NULL);
	while (1) {
		char* line = recvline(sock);
		int len = strlen(line);
		/* fprintf(stderr, "%s\n", line); */
		free(line);
		if (len == 0) break;
	}
	close_socket(sock);
	sock = 0;

leave:
	if (title) free(title);
	if (message) free(message);
	if (icon) free(icon);

#ifdef _WIN32
	WSACleanup();
#endif
	return (sock == 0) ? 0 : -1;
}
