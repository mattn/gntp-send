#ifndef _TCP_H_
#define _TCP_H_

#ifdef _MSC_VER
#define __attribute__(x)
#endif

void growl_tcp_write(int sock, const char *const format, ...) __attribute__ ((format (printf, 2, 3)));
char* growl_tcp_read(int sock);
int growl_tcp_open(const char* server);
void growl_tcp_close(int sock);

#endif /* _TCP_H_ */
