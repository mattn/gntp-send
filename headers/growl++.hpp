class Growl 
{
	private:
		char *server;
		char *password;
	public:
		Growl(const char *const _password);
		Growl(const char *const _server, const char *const _password);
		void  notify(const char *const title, const char *const message);
};


