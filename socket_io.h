#ifndef Socket_IO_Header
#define Socket_IO_Header

#ifdef  __cplusplus
extern "C" {
#endif

int init_server(int port);
int accept_connection(int serversock);
int handle_client(int sock, char* buffer, unsigned char* plength);
void shut_server(int serversock, int clientsock);

#ifdef  __cplusplus
}
#endif

#endif
