#ifndef _GROWL_H_
#define _GROWL_H_


#ifdef __cplusplus
extern "C" {
#endif 


int growl( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );
int growl_udp( const char *const server,const char *const appname,const char *const notify,const char *const title, const char *const message ,
                                const char *const icon , const char *const password , const char *url );


#ifdef __cplusplus
}
#endif 


#endif /* _GROWL_H_ */
