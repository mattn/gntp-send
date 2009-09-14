/**
 * msvc: cl gntp-send.c
 * mingw32: gcc -o gntp-send.exe gntp-send.c -lws2_32
 * gcc: gcc -o gntp-send gntp-send.c
 */
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

int	opterr = 1;
int	optind = 1;
int	optopt;
char *optarg;

int getopts(int argc, char** argv, char* opts) {
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1) {
		if(optind >= argc ||
				argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

int main(int argc, char* argv[]) {
	int sock = -1;
	int c;
	char* server = "127.0.0.1:23053";
	char* password = NULL;
	char* appname = "gntp-send";
	char* notify = "gntp-send notify";
	char* title = NULL;
	char* message = NULL;
	char* icon = NULL;
	char* salt;
	char* salthash;
	char* keyhash;
	char* authheader = NULL;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) goto leave;
	setlocale(LC_CTYPE, "");
#endif

	opterr = 0;
	while ((c = getopts(argc, argv, "a:n:s:p:") != -1)) {
		switch (optopt) {
		case 'a': appname = optarg; break;
		case 'n': notify = optarg; break;
		case 's': server = optarg; break;
		case 'p': password = optarg; break;
		case '?': break;
		default:
			argc = 0;
			break;
		}
		optarg = NULL;
	}

	if ((argc - optind) != 2 && (argc - optind) != 3) {
		fprintf(stderr, "%s: [-a APPNAME] [-n NOTIFY] [-s SERVER:PORT] [-p PASSWORD] title message [icon]", argv[0]);
		exit(1);
	}

	title = string_to_utf8_alloc(argv[optind]);
	message = string_to_utf8_alloc(argv[optind + 1]);
	if ((argc - optind) == 3) icon = string_to_utf8_alloc(argv[optind + 2]);

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
	if (title) free(title);
	if (message) free(message);
	if (icon) free(icon);
	if (authheader) free(authheader);

#ifdef _WIN32
	WSACleanup();
#endif
	return (sock == 0) ? 0 : -1;
}
