#ifndef _TCP_H_
#define _TCP_H_

#ifdef _MSC_VER
#pragma warning( disable : 4996 4244 )
#define __attribute__(x)
#endif
void growl_tcp_write_raw( int sock, const unsigned char * data, const int data_length );
void growl_tcp_write( int sock , const char *const format , ... ) __attribute__ ((format (printf, 2, 3)));
char* growl_tcp_read(int sock);
int growl_tcp_open(const char* server);
void growl_tcp_close(int sock);
int growl_tcp_datagram( const char *server , const unsigned char *data , const int data_length );

#endif /* _TCP_H_ */

/* vim:set et sw=2 ts=2 ai: */
