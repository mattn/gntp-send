void sendline(int sock, const char* str, const char* val);
char* recvline(int sock);
int create_socket(const char* server);
void close_socket(int sock);
