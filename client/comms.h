#ifndef COMMS_H_
#define COMMS_H_

#include <stdint.h>

// Opens a socket
// Returns socket descriptor on success; -1 on failure
int openSocket();

// Closes a specified socket
// Returns 0 on success; -1 on failure
int closeSocket(int socket);

// Connects a specified socket to IPv4 address and port combo
// Returns 0 on success; -1 on failure
int connectToAddr(int socket, const char* host, uint16_t port);

// Writes [size] bytes from buffer to specified socket
// Returns number of bytes written on success; -1 on failure
int writeData(int dest, void* buffer, uint32_t size);

// Reads [size] bytes from specified socket to buffer
// Returns number of bytes read on success; -1 on failure
int readData(int src, void* buffer, uint32_t size);

#endif // COMMS_H_