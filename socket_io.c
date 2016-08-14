/*
 * TCP/IP Socket, Sung Chul Lee
 */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>

static struct sockaddr_in g_server_addr,g_client_addr;

int init_server(int port)
{
	int serversock,flags;

	if((serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("failed to create socket\n");
		exit(1);
	}

	if((flags = fcntl(serversock, F_GETFL, 0)) < 0) 
	{ 
		printf("failed to retrieve socket descriptor\n");
		exit(1);
	}

	if(fcntl(serversock, F_SETFL, flags | O_NONBLOCK) < 0)
	{ 
		printf("failed to set socket in non-blocking mode\n");
		exit(1);
	}

	memset(&g_server_addr, 0, sizeof(g_server_addr));
	g_server_addr.sin_family=AF_INET;			
	g_server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	g_server_addr.sin_port=htons(port);				

	if(bind(serversock, (struct sockaddr *) &g_server_addr, sizeof(g_server_addr)) < 0)
	{
		printf("failed to bind the server socket\n");
		return NULL;
	}

	if(listen(serversock, 5) < 0)
	{
		printf("failed to listen on server socket\n");
		exit(1);
	}
	printf("opening server (port %d)\n", port);
	return serversock;
}

void shut_server(int serversock, int clientsock)
{
	close(clientsock);
	close(serversock);
}

int accept_connection(int serversock)
{
	int clientsock;
	unsigned int clientlen=sizeof(g_client_addr);

	clientsock=accept(serversock, (struct sockaddr *)&g_client_addr, &clientlen);

	/*
	if(clientsock != -1)
		printf("established connection to (%s)\n",inet_ntoa(g_client_addr.sin_addr));
	*/

	return clientsock;
}

int handle_client(int sock, char* buffer, unsigned char* plength)
{
	int i,received=-1;
	unsigned char length=0;

	*plength=0;

	received=recv(sock, buffer, 1, MSG_DONTWAIT);
	if(received > 0)
	{
		length=(unsigned char)buffer[0];
		for(i=0; i < length; i++)
		{
			received=recv(sock, &buffer[i], 1, 0);
			if(received < 1)
			{
				/* printf("connection lost (discarding data)\n"); */
				close(sock);
				return -1;
			}
		}
		*plength=length;
	}
	else if(received == 0)
	{
		/* printf("connection lost\n"); */
		close(sock);
		sock=-1;
	}
	return sock;
}
