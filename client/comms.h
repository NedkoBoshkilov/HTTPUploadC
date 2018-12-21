#ifndef COMMS_H_
#define COMMS_H_

#include <stdint.h>

/* 
 * Opens a socket to be used for communication
 * Returns socket descriptor on success; -1 on failure
 */
int open_socket ();

/* 
 * Closes a specified socket
 * Returns 0 on success; -1 on failure
 */
int close_socket (int _socket);

/*
 * Connects a specified socket to IPv4 address and port combo
 * Returns 0 on success; -1 on failure
 */
int connect_to_addr (int _socket, const char *_host, uint16_t _port);

/*
 * Writes [size] bytes from buffer to specified socket
 * Returns number of bytes written on success; -1 on failure
 */
int64_t write_data (int _dest, void *_buffer, uint32_t _size);

/*
 * Reads [size] bytes from specified socket to buffer
 * ms - timeout in ms after which it's assumed there's no data
 * Returns number of bytes read on success; -1 on failure
 */
int64_t read_data (int _src, void *_buffer, uint32_t _size, int _ms);

#endif // COMMS_H_
