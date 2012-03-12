#ifndef GROWLXX_HPP_
#define GROWLXX_HPP_

#include <growl.h>

#ifdef _WIN32
  #ifndef GROWL_CPP_STATIC
    #ifdef GROWL_CPP_DLL
      #define GROWL_CPP_EXPORT __declspec(dllexport)
    #else
      #define GROWL_CPP_EXPORT __declspec(dllimport)
    #endif
  #else
    #define GROWL_CPP_EXPORT
  #endif
#else
  #define GROWL_CPP_EXPORT
#endif //_WIN32

enum Growl_Protocol { GROWL_UDP , GROWL_TCP };

class GROWL_CPP_EXPORT Growl {
private:
  char *server;
  char *password;
  Growl_Protocol protocol;
  char *application;
  void Register(const char **const _notifications, const int _notifications_count, const char* const icon = 0 );
public:
  Growl(const Growl_Protocol _protocol, const char *const _password, const char* const _appliciation, const char **const _notifications, const int _notifications_count);
  Growl(const Growl_Protocol _protocol, const char *const _server, const char *const _password, const char *const _application, const char **const _notifications, const int _notifications_count);
  ~Growl();
  void Notify(const char *const notification, const char *const title, const char* const message);
  void Notify(const char *const notification, const char *const title, const char* const message, const char *const url, const char *const icon);
  void Notify(const char *const notification, const char *const title, const char* const message, const char *const url, const unsigned char *const icon_data, const long icon_size);
};

#endif // GROWLXX_HPP_

/* vim:set et sw=2 ts=2 ai: */
