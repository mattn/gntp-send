#include <growl++.hpp>
#include <growl.h>
#include <stdio.h>
#include <string.h>

Growl::Growl(const char *const _password)
{
	server = strdup("localhost");
	password = strdup(_password);
}


Growl::Growl(const char *const _server, const char *const _password)
{
	server = strdup(_server);
	password = strdup(_password);
}


void Growl::notify(const char *const title, const char* const message)
{
	printf("notify called\n");
	growl_udp( server , "app" , "why" , title , message , NULL , password , NULL );
}
