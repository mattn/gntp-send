#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"

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

int growl(char *server,char *appname,char *notify,char *title, char *message , char *icon , char *password )
{		
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

	sock = create_socket(server);
	if (sock == -1) goto leave;
    
	sendline(sock, "GNTP/1.0 REGISTER NONE", authheader);
	sendline(sock, "Application-Name: ", appname);
	sendline(sock, "Notifications-Count: 1", NULL);
	sendline(sock, "", NULL);
	sendline(sock, "Notification-Name: ", notify);
	sendline(sock, "Notification-Display-Name: ", notify);
	sendline(sock, "Notification-Enabled: True", NULL);
	sendline(sock, "", NULL);
	while (1) {
		char* line = recvline(sock);
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
	close_socket(sock);

	sock = create_socket(server);
	if (sock == -1) goto leave;

	sendline(sock, "GNTP/1.0 NOTIFY NONE", authheader);
	sendline(sock, "Application-Name: ", appname);
	sendline(sock, "Notification-Name: ", notify);
	sendline(sock, "Notification-Title: ", title);
	sendline(sock, "Notification-Text: ", message);
	if (icon) sendline(sock, "Notification-Icon: ", icon);
	sendline(sock, "", NULL);
	while (1) {
		char* line = recvline(sock);
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
	close_socket(sock);
	sock = 0;

leave:
	if (authheader) free(authheader);

#ifdef _WIN32
	WSACleanup();
#endif
	return (sock == 0) ? 0 : -1;
}
