#include <growl++.hpp>

int main(int argc, char **argv)
{
	Growl *growl = new Growl(GROWL_UDP,"password","gntp_send++","bob",1);
	growl->Notify("bob","title","message");

	delete(growl);

	return 0;
}
