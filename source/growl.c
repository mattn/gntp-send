#ifdef _WIN32
#include <windows.h>
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"

#define __str__(x) (x ? x : "")

static const char hex_table[] = "0123456789ABCDEF";
static char* string_to_hex_alloc(const char* str, int len) {
	int n, l;
	char* tmp = (char*)malloc(len * 2 + 1);
	memset(tmp, 0, len * 2 + 1);
    for (l = 0, n = 0; l < len; l++) {
        tmp[n++] = hex_table[(str[l] & 0xF0) >> 4];
        tmp[n++] = hex_table[str[l] & 0x0F];
    }
    return tmp;
}

char* gen_salt_alloc(int count) {
	char* salt = (char*)malloc(count + 1);
	int n;
	for (n = 0; n < count; n++) salt[n] = (((int)rand()) % 255) + 1;
	salt[n] = 0;
	return salt;
}

char* gen_password_hash_alloc(const char* password, const char* salt) {
	md5_context md5ctx;
	char md5tmp[20];
	char* md5digest;

	memset(md5tmp, 0, sizeof(md5tmp));
	md5_starts(&md5ctx);
	md5_update(&md5ctx, (uint8*)password, strlen(password));
	md5_update(&md5ctx, (uint8*)salt, strlen(salt));
	md5_finish(&md5ctx, (uint8*)md5tmp);

	md5_starts(&md5ctx);
	md5_update(&md5ctx, (uint8*)md5tmp, 16);
	md5_finish(&md5ctx, (uint8*)md5tmp);
	md5digest = string_to_hex_alloc(md5tmp, 16);

	return md5digest;
}

EXPORT
int growl(const char *const server, const char *const appname,
		const char *const notify, const char *const title,
		const char *const message, const char *const icon,
		const char *const password, const char *url) {		
	int sock = -1;
	char* salt;
        char* salthash;
        char* keyhash;
        char* authheader = NULL;

	if (password) {
		srand(time(NULL));
		salt = gen_salt_alloc(8);
		keyhash = gen_password_hash_alloc(password, salt);
		salthash = string_to_hex_alloc(salt, 8);
		free(salt);
		authheader = (char*)malloc(strlen(keyhash) + strlen(salthash) + 7);
		sprintf(authheader, " MD5:%s.%s", keyhash, salthash);
		free(salthash);
	}

	sock = growl_tcp_open(server);
	if (sock == -1) goto leave;
    
	growl_tcp_write(sock, "GNTP/1.0 REGISTER NONE %s", __str__(authheader));
	growl_tcp_write(sock, "Application-Name: %s ", __str__(appname));
	growl_tcp_write(sock, "Notifications-Count: 1");
	growl_tcp_write(sock, "" );
	growl_tcp_write(sock, "Notification-Name: %s", __str__(notify));
	growl_tcp_write(sock, "Notification-Display-Name: %s", __str__(notify));
	growl_tcp_write(sock, "Notification-Enabled: True" );
	growl_tcp_write(sock, "" );
	while (1) {
		char* line = growl_tcp_read(sock);
		int len = strlen(line);
		/* fprintf(stderr, "%s\n", line); */
		if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) {
			fprintf(stderr, "failed to register notification\n");
			free(line);
			goto leave;
		}
		free(line);
		if (len == 0) break;
	}
	growl_tcp_close(sock);

	sock = growl_tcp_open(server);
	if (sock == -1) goto leave;

	growl_tcp_write(sock, "GNTP/1.0 NOTIFY NONE %s", __str__(authheader));
	growl_tcp_write(sock, "Application-Name: %s", __str__(appname));
	growl_tcp_write(sock, "Notification-Name: %s", __str__(notify));
	growl_tcp_write(sock, "Notification-Title: %s", __str__(title));
	growl_tcp_write(sock, "Notification-Text: %s", __str__(message));
	if (icon) growl_tcp_write(sock, "Notification-Icon: %s", __str__(icon));
	if (url) growl_tcp_write(sock, "Notification-Callback-Target: %s", __str__(url));

	growl_tcp_write(sock, "");
	while (1) {
		char* line = growl_tcp_read(sock);
		int len = strlen(line);
		/* fprintf(stderr, "%s\n", line); */
		if (strncmp(line, "GNTP/1.0 -ERROR", 15) == 0) {
			fprintf(stderr, "failed to post notification\n");
			free(line);
			goto leave;
		}
		free(line);
		if (len == 0) break;
	}
	growl_tcp_close(sock);
	sock = 0;

leave:
	if (authheader) free(authheader);

	return (sock == 0) ? 0 : -1;
}

#ifdef _WIN32
EXPORT
void GrowlNotify(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow) {
	char* server = "127.0.0.1:23053";
	char* password = NULL;
	char* appname = "gntp-send";
	char* notify = "gntp-send notify";
	char* title = NULL;
	char* message = NULL;
	char* icon = NULL;
	char* url = NULL;
	char* first = strdup(lpszCmdLine);
	char* ptr = first;
	int rc;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) return;
	#define SKIP(x)	while (*x && *x != ' ') x++; if (*x == ' ') *x++ = 0;
	server = ptr;  SKIP(ptr);
	appname = ptr; SKIP(ptr);
	notify = ptr;  SKIP(ptr);
	title = ptr;   SKIP(ptr);
	message = ptr; SKIP(ptr);
	icon = ptr;    SKIP(ptr);
	url = ptr;     SKIP(ptr);
	rc = growl(server,appname,notify,title,message,icon,password,url);
	WSACleanup();
	free(ptr);
}
#endif
