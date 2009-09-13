/**
 * msvc: cl gntp-send.c
 * mingw32: gcc -o gntp-send.exe gntp-send.c -lws2_32
 * gcc: gcc -o gntp-send gntp-send.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifdef _WIN32
#include <locale.h>
#include <winsock2.h>
#pragma comment (lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif

#ifndef uint64
# ifdef _WIN32
#  ifdef __GNUC__
#   define uint64 unsigned __int64
#  else
#   define uint64 unsigned _int64
#  endif
# else
#  define uint64 unsigned long long
# endif
#endif

#ifndef byte
# define byte unsigned char
#endif

typedef struct {
	uint32 total[2];
	uint32 state[4];
	uint8 buffer[64];
} md5_context;

static void md5_starts(md5_context *ctx);
static void md5_update(md5_context *ctx, const uint8 *input, uint32 length);
static void md5_finish(md5_context *ctx, uint8 digest[16]);

#define GET_UINT32(n, b, i)     n = b[i] + (b[i+1]<<8) + (b[i+2]<<16) + (b[i+3]<<24)
#define PUT_UINT32(n, b, i)     do { b[i] = n; b[i+1] = n >> 8; b[i+2] = n >> 16; b[i+3] = n >> 24; } while(0)

static void md5_starts(md5_context *ctx) {
	ctx->total[0] = 0;
	ctx->total[1] = 0;

	ctx->state[0] = 0x67452301;
	ctx->state[1] = 0xEFCDAB89;
	ctx->state[2] = 0x98BADCFE;
	ctx->state[3] = 0x10325476;
}

static void md5_process(md5_context *ctx, const uint8 data[64]) {
	uint32 X[16], A, B, C, D;

	GET_UINT32(X[0],  data,  0);
	GET_UINT32(X[1],  data,  4);
	GET_UINT32(X[2],  data,  8);
	GET_UINT32(X[3],  data, 12);
	GET_UINT32(X[4],  data, 16);
	GET_UINT32(X[5],  data, 20);
	GET_UINT32(X[6],  data, 24);
	GET_UINT32(X[7],  data, 28);
	GET_UINT32(X[8],  data, 32);
	GET_UINT32(X[9],  data, 36);
	GET_UINT32(X[10], data, 40);
	GET_UINT32(X[11], data, 44);
	GET_UINT32(X[12], data, 48);
	GET_UINT32(X[13], data, 52);
	GET_UINT32(X[14], data, 56);
	GET_UINT32(X[15], data, 60);

#define S(x, n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a, b, c, d, k, s, t)                    \
	{                                                 \
		a += F(b,c,d) + X[k] + t; a = S(a,s) + b; \
	}

	A = ctx->state[0];
	B = ctx->state[1];
	C = ctx->state[2];
	D = ctx->state[3];

#define F(x, y, z) (z ^ (x & (y ^ z)))

	P(A, B, C, D,  0,  7, 0xD76AA478);
	P(D, A, B, C,  1, 12, 0xE8C7B756);
	P(C, D, A, B,  2, 17, 0x242070DB);
	P(B, C, D, A,  3, 22, 0xC1BDCEEE);
	P(A, B, C, D,  4,  7, 0xF57C0FAF);
	P(D, A, B, C,  5, 12, 0x4787C62A);
	P(C, D, A, B,  6, 17, 0xA8304613);
	P(B, C, D, A,  7, 22, 0xFD469501);
	P(A, B, C, D,  8,  7, 0x698098D8);
	P(D, A, B, C,  9, 12, 0x8B44F7AF);
	P(C, D, A, B, 10, 17, 0xFFFF5BB1);
	P(B, C, D, A, 11, 22, 0x895CD7BE);
	P(A, B, C, D, 12,  7, 0x6B901122);
	P(D, A, B, C, 13, 12, 0xFD987193);
	P(C, D, A, B, 14, 17, 0xA679438E);
	P(B, C, D, A, 15, 22, 0x49B40821);

#undef F

#define F(x, y, z) (y ^ (z & (x ^ y)))

	P(A, B, C, D,  1,  5, 0xF61E2562);
	P(D, A, B, C,  6,  9, 0xC040B340);
	P(C, D, A, B, 11, 14, 0x265E5A51);
	P(B, C, D, A,  0, 20, 0xE9B6C7AA);
	P(A, B, C, D,  5,  5, 0xD62F105D);
	P(D, A, B, C, 10,  9, 0x02441453);
	P(C, D, A, B, 15, 14, 0xD8A1E681);
	P(B, C, D, A,  4, 20, 0xE7D3FBC8);
	P(A, B, C, D,  9,  5, 0x21E1CDE6);
	P(D, A, B, C, 14,  9, 0xC33707D6);
	P(C, D, A, B,  3, 14, 0xF4D50D87);
	P(B, C, D, A,  8, 20, 0x455A14ED);
	P(A, B, C, D, 13,  5, 0xA9E3E905);
	P(D, A, B, C,  2,  9, 0xFCEFA3F8);
	P(C, D, A, B,  7, 14, 0x676F02D9);
	P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F

#define F(x, y, z) (x ^ y ^ z)

	P(A, B, C, D,  5,  4, 0xFFFA3942);
	P(D, A, B, C,  8, 11, 0x8771F681);
	P(C, D, A, B, 11, 16, 0x6D9D6122);
	P(B, C, D, A, 14, 23, 0xFDE5380C);
	P(A, B, C, D,  1,  4, 0xA4BEEA44);
	P(D, A, B, C,  4, 11, 0x4BDECFA9);
	P(C, D, A, B,  7, 16, 0xF6BB4B60);
	P(B, C, D, A, 10, 23, 0xBEBFBC70);
	P(A, B, C, D, 13,  4, 0x289B7EC6);
	P(D, A, B, C,  0, 11, 0xEAA127FA);
	P(C, D, A, B,  3, 16, 0xD4EF3085);
	P(B, C, D, A,  6, 23, 0x04881D05);
	P(A, B, C, D,  9,  4, 0xD9D4D039);
	P(D, A, B, C, 12, 11, 0xE6DB99E5);
	P(C, D, A, B, 15, 16, 0x1FA27CF8);
	P(B, C, D, A,  2, 23, 0xC4AC5665);

#undef F

#define F(x, y, z) (y ^ (x | ~z))

	P(A, B, C, D,  0,  6, 0xF4292244);
	P(D, A, B, C,  7, 10, 0x432AFF97);
	P(C, D, A, B, 14, 15, 0xAB9423A7);
	P(B, C, D, A,  5, 21, 0xFC93A039);
	P(A, B, C, D, 12,  6, 0x655B59C3);
	P(D, A, B, C,  3, 10, 0x8F0CCC92);
	P(C, D, A, B, 10, 15, 0xFFEFF47D);
	P(B, C, D, A,  1, 21, 0x85845DD1);
	P(A, B, C, D,  8,  6, 0x6FA87E4F);
	P(D, A, B, C, 15, 10, 0xFE2CE6E0);
	P(C, D, A, B,  6, 15, 0xA3014314);
	P(B, C, D, A, 13, 21, 0x4E0811A1);
	P(A, B, C, D,  4,  6, 0xF7537E82);
	P(D, A, B, C, 11, 10, 0xBD3AF235);
	P(C, D, A, B,  2, 15, 0x2AD7D2BB);
	P(B, C, D, A,  9, 21, 0xEB86D391);

#undef F

	ctx->state[0] += A;
	ctx->state[1] += B;
	ctx->state[2] += C;
	ctx->state[3] += D;
}

static void md5_update(md5_context *ctx, const uint8 *input, uint32 length) {
	uint32 left, fill;

	if (!length)
		return;

	left = ctx->total[0] & 0x3F;
	fill = 64 - left;

	ctx->total[0] += length;
	ctx->total[0] &= 0xFFFFFFFF;

	if (ctx->total[0] < length)
		ctx->total[1]++;

	if (left && length >= fill) {
		memcpy((void *)(ctx->buffer + left), (const void *)input, fill);
		md5_process(ctx, ctx->buffer);
		length -= fill;
		input  += fill;
		left = 0;
	}

	while (length >= 64) {
		md5_process(ctx, input);
		length -= 64;
		input  += 64;
	}

	if (length) {
		memcpy((void *)(ctx->buffer + left), (const void *)input, length);
	}
}

static const uint8 md5_padding[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static void md5_finish(md5_context *ctx, uint8 digest[16]) {
	uint32 last, padn;
	uint32 high, low;
	uint8 msglen[8];

	high = (ctx->total[0] >> 29) | (ctx->total[1] << 3);
	low  = (ctx->total[0] <<  3);

	PUT_UINT32(low,  msglen, 0);
	PUT_UINT32(high, msglen, 4);

	last = ctx->total[0] & 0x3F;
	padn = (last < 56) ? (56 - last) : (120 - last);

	md5_update(ctx, md5_padding, padn);
	md5_update(ctx, msglen, 8);

	PUT_UINT32(ctx->state[0], digest,  0);
	PUT_UINT32(ctx->state[1], digest,  4);
	PUT_UINT32(ctx->state[2], digest,  8);
	PUT_UINT32(ctx->state[3], digest, 12);
}

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

static void close_socket(int sock) {
#ifdef _WIN32
	if (sock < 0) closesocket(sock);
#else
	if (sock < 0) close(sock);
#endif
}

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
