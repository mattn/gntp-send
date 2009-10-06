#include <growl++.hpp>

int main(int argc, char **argv)
{
	Growl *growl = new Growl("password");
	growl->notify("title","message");

	Growl *growl2 = new Growl("localhost","password");
	growl2->notify("tit","mes");

	Growl *growl3 = new Growl("localhost:555");
	growl3->notify("t","m");

	return 1;
}
