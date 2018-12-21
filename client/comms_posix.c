#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>

#include "comms.h"

int
open_socket () {
	int result;

	result = socket (AF_INET, SOCK_STREAM, 0);
	return result;
}

int
connect_to_addr (int socket, const char *host, uint16_t port) {
	int result;
	struct in_addr ipv4_addr;
	struct sockaddr_in server_address;

	result = inet_aton (host, &ipv4_addr);
	if (1 != result) {
		return -1;
	}

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = ipv4_addr.s_addr;
	server_address.sin_port = htons (port);

	result =
	  connect (socket, (const struct sockaddr *)&server_address,
		   sizeof (server_address));

	return result;
}

int
close_socket (int socket) {
	int result;

	result = close (socket);
	return result;
}

int64_t
write_data (int dest, void *buffer, uint32_t size) {
	int64_t result;

	result = write (dest, buffer, size);
	return result;
}

int64_t
read_data (int src, void *buffer, uint32_t size, int ms) {
	struct timeval timeout;
	fd_set set;
	int64_t result;

	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = ((time_t) ms % 1000) * 1000;
	FD_ZERO (&set);
	FD_SET (src, &set);

	result = select (src + 1, &set, NULL, NULL, &timeout);
	if (result > 0) {
		result = read (src, buffer, size);
	}
	return result;
}
