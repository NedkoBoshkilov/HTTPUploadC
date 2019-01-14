#ifndef COMMS_H_
#define COMMS_H_

#include <stdint.h>

/*
 * Opens a port to be used for communication to the device
 * Returns a file descriptor on success; -1 on failure
 * 
 * com_port - port on which to send the commands
 */
int open_port (const char *_com_port);

/*
 * Closes a port via its file descriptor
 * Returns 0 on success; -1 on failure
 * 
 * fd - file descriptor of the port to be closed
 */
int close_port (int _fd);

/* 
 * Opens a socket to be used for communication
 * Returns socket descriptor on success; -1 on failure
 * 
 * fd - file descriptor of open port on which to send the commands
 * ms - timeout in ms after which it's assumed there's no data
 */
int open_socket (int _fd, int _ms);

/* 
 * Closes a specified socket
 * Returns 0 on success; -1 on failure
 * 
 * fd - file descriptor of open port on which to send the commands
 * socket - socket descriptor to close
 * ms - timeout in ms after which it's assumed there's no data
 */
int close_socket (int _fd, int _socket, int _ms);

/*
 * Connects a specified socket to IPv4 address and port combo
 * Returns 0 on success; -1 on failure
 * 
 * fd - file descriptor of open port on which to send the commands
 * socket - socket descriptor on which to be used for connection
 * host - remote host
 * port - remote port
 * ms - timeout in ms after which it's assumed there's no data
 */
int connect_to_addr (int _fd, int _socket, const char *_host, const char *_port,
		     int _ms);

/*
 * Writes [size] bytes from buffer to specified socket
 * Returns number of bytes written on success; -1 on failure
 * 
 * fd - file descriptor of open port on which to send the commands
 * socket - socket descriptor on which to send data
 * buffer - pointer to the data to be send
 * size - number of bytes to be sent
 * ms - timeout in ms after which it's assumed there's no data
 */
int64_t write_data (int _fd, int _socket, void *_buffer, uint32_t _size,
		    int _ms);

/*
 * Reads [size] bytes from specified socket to buffer
 * Returns number of bytes read on success; -1 on failure
 * 
 * fd - file descriptor of open port on which to send the commands
 * socket - socket descriptor on which to receive data
 * buffer - pointer to where the data will be stored
 * size - number of bytes to be read
 * ms - timeout in ms after which it's assumed there's no data
 */
int64_t read_data (int _fd, int _socket, void *_buffer, uint32_t _size,
		   int _ms);

#endif // COMMS_H_
