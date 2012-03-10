#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
void
growl_perror(const char* s) {
  char buf[200];

  if (FormatMessage(
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      buf,
      sizeof(buf),
      NULL) != 0)
    fprintf(stderr, "%s: %s\n", s, buf);
}
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#define growl_perror(x) perror(x)
#endif

#include "tcp.h"

static int
growl_tcp_parse_hostname(
    const char *const server,
    int default_port,
    struct sockaddr_in *const sockaddr);

void
growl_tcp_write_raw(
    int sock,
    const unsigned char * data,
    const int data_length) {

  send(sock, (const char*) data, data_length, 0);
}

void
growl_tcp_write(int sock, const char *const format, ...) {
  int length;
  char *output;
  char *stop;

  va_list ap;

  va_start(ap, format);
  length = vsnprintf(NULL, 0, format, ap);
  va_end(ap);

  va_start(ap,format);
  output = (char*)malloc(length+1);
  if (!output) {
    va_end(ap);
    return;
  }
  vsnprintf(output, length+1, format, ap);
  va_end(ap);

  while ((stop = strstr(output, "\r\n"))) strcpy(stop, stop + 1);

  send(sock, output, length, 0);
  send(sock, "\r\n", 2, 0);

  free(output);
}

char*
growl_tcp_read(int sock) {
  const int growsize = 80;
  char c = 0;
  char* line = (char*) malloc(growsize);
  if (line) {
    int len = growsize, pos = 0;
    char* newline;
    while (line) {
      if (recv(sock, &c, 1, 0) <= 0) break;
      if (c == '\r') continue;
      if (c == '\n') break;
      line[pos++] = c;
      if (pos >= len) {
        len += growsize;
        newline = (char*) realloc(line, len);
        if (!newline) {
          free(line);
          return NULL;
        }
        line = newline;
      }
    }
    line[pos] = 0;
  }
  return line;
}

int
growl_tcp_open(const char* server) {
  int sock = -1;
#ifdef _WIN32
  char on;
#else
  int on;
#endif
  struct sockaddr_in serv_addr = {0};

  if (growl_tcp_parse_hostname(server, 23053, &serv_addr) == -1) {
    return -1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    growl_perror("create socket");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
    growl_perror("connect");
    growl_tcp_close(sock);
    return -1;
  }

  on = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1) {
    growl_perror("setsockopt");
    growl_tcp_close(sock);
    return -1;
  }

  return sock;
}

void
growl_tcp_close(int sock) {
#ifdef _WIN32
  if (sock != -1) closesocket(sock);
#else
  if (sock != -1) close(sock);
#endif
}

int
growl_tcp_parse_hostname(
    const char *const server,
    int default_port,
    struct sockaddr_in *const sockaddr) {

  char *hostname = strdup(server);
  char *port = strchr(hostname, ':');
  struct hostent* host_ent;
  if (port != NULL) {
    *port = '\0';
    port++;
    default_port = atoi(port);
  }

  host_ent = gethostbyname(hostname);
  if (host_ent == NULL) {
    growl_perror("gethostbyname");
    free(hostname);
    return -1;
  }

  sockaddr->sin_family = AF_INET;
  memcpy(&sockaddr->sin_addr, host_ent->h_addr, host_ent->h_length);
  sockaddr->sin_port = htons(default_port);

  free(hostname);
  return 0;
}

int
growl_tcp_datagram(
    const char *server,
    const unsigned char *data,
    const int data_length) {

  struct sockaddr_in serv_addr = {0};
  int sock = 0;

  if (growl_tcp_parse_hostname(server, 9887, &serv_addr) == -1) {
    return -1;
  }

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1) {
    return -1;
  }

  if (sendto(sock, (char*)data, data_length, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) > 0) {
    return 0;
  } else {
    return 1;
  }
}

/* vim:set et sw=2 ts=2 ai: */
