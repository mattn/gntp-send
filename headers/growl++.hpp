#include <stdio.h>

enum Growl_Protocol { GROWL_UDP , GROWL_TCP };

class Growl 
{
	private:
		char *server;
		char *password;
		char *application;
		Growl_Protocol protocol;
		void Register(const char **const _notifications, const int _notifications_count);
	public:
		Growl(const Growl_Protocol _protocol, const char *const _password, const char* const _appliciation, const char **const _notifications, const int _notifications_count);
		Growl(const Growl_Protocol _protocol, const char *const _server, const char *const _password, const char *const _application, const char **const _notifications, const int _notifications_count);
		~Growl();
		void Notify(const char *const notification, const char *const title, const char* const message);
		void Notify(const char *const notification, const char *const title, const char* const message, const char *const url, const char *const icon);

};


