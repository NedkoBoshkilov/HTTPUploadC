#include "comms.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int openSocket()
{
	int result = 0;
	int errorValue = 0;
    
    result = socket(AF_INET, SOCK_STREAM, 0);
	
	return result;
}

int connectToAddr(int socket, const char* host, uint16_t port)
{
	int result = 0;
	struct sockaddr_in serverAddress;
	struct in_addr ipv4Addr;

	result = inet_aton(host, &ipv4Addr);
	if(result != 1)
	{
		return -1;
	}

    serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = ipv4Addr.s_addr;
    serverAddress.sin_port = htons(port);

	result = connect(socket, (const struct sockaddr*) &serverAddress, sizeof(serverAddress));
	
	return result;
}

int closeSocket(int socket)
{
	int result = 0;
	
	result = close(socket);
	
	return result;
}

int writeData(int dest, void* buffer, uint32_t size)
{
	int result = 0;
	
	result = write(dest, buffer, size);
	
	return result;
}

int readData(int src, void* buffer, uint32_t size)
{
	int result = 0;
	
	result = read(src, buffer, size);
	
	return result;
}
