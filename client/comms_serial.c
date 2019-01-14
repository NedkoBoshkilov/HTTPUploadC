#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "comms.h"

#define DEFAULT_TIMEOUT 2500

static int hex_to_decimal_convert (const char *_str);
static int read_port (int _fd, void *_buffer, int _size, int _ms);
static int read_buffered_port (int _fd, void *_buffer, int _size, int _ms);
static int read_next_block (int _fd, void *_buffer, int _ms);

/*
 * Command response format
 * \r\n[response]\r\n
 * \r\nOK\r\n
 * 
 * or
 * 
 * \r\nERROR\r\n
 */

static int
hex_to_decimal_convert (const char *str) {
	int result;

	result = 0;

	if (NULL == str) {
		return -1;
	}

	for (int i = 0; i < 2; ++i) {
		if ((str[i] >= '0') && (str[i] <= '9')) {
			result = result << 4;
			result += str[i] - '0';
		} else if ((str[i] >= 'a') && (str[i] <= 'f')) {
			result = result << 4;
			result += str[i] - 'a' + 10;
		} else if ((str[i] >= 'A') && (str[i] <= 'F')) {
			result = result << 4;
			result += str[i] - 'A' + 10;
		} else {
			return -1;
		}
	}
	return result;
}

static int
read_port (int fd, void *buffer, int size, int ms) {
	struct timeval timeout;
	fd_set set;
	int result;

	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = ((time_t) ms % 1000) * 1000;
	FD_ZERO (&set);
	FD_SET (fd, &set);

	result = select (fd + 1, &set, NULL, NULL, &timeout);
	if (result > 0) {
		result = read (fd, buffer, size);
	}
	return result;
}

static int
read_buffered_port (int fd, void *buffer, int size, int ms) {
	int total_read;
	int bytes_read;

	if (NULL == buffer) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	total_read = 0;
	while (0 !=
	       (bytes_read =
		read_port (fd, buffer + total_read, size - total_read, ms))) {
		if (bytes_read < 0) {
			return -1;
		}
		total_read += bytes_read;
	}
	return total_read;
}

static int
read_next_block (int fd, void *buffer, int ms) {
	int temp;

	/* +NSONMI,[socket],[length],[data] */
	const char read_response[8 + 1] = "+NSONMI:";
	const char eof_response[8 + 1] = "+NSOCLI:";
	char discard;
	int bytes_read;
	int idx;
	char block_size_str[5];
	int result;

	if (fd < 0) {
		return -1;
	}

	if (NULL == buffer) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	/* Discard \r\n+NSONMI,[socket], */
	for (temp = 0; temp < 12; ++temp) {
		bytes_read = read_buffered_port (fd, &discard, 1, ms);
		if (bytes_read < 0) {
			return -1;
		} else if (0 == bytes_read) {
			return 0;
		}

		/* TODO: SOCKET CLOSE MESSAGE */

		if ((temp > 1) && (temp < 10) &&
		    ((read_response[temp - 2] != discard) &&
		     ((eof_response[temp - 2] != discard)))) {
			return -1;
		}
	}

	idx = 0;
	do {
		bytes_read =
		  read_buffered_port (fd, &(block_size_str[idx]), 1, ms);
		if (bytes_read < 0) {
			return -1;
		} else if (0 == bytes_read) {
			return 0;
		}
		++idx;
	}
	while (block_size_str[idx - 1] != ',');

	idx = 0;
	result = 0;
	while (block_size_str[idx] != ',') {
		result *= 10;
		result += block_size_str[idx] - '0';
		++idx;
	}

	/* Read the entire hex buffer */
	for (temp = 0; temp < result; ++temp) {
		if (2 != read_buffered_port (fd, block_size_str, 2, ms)) {
			return -1;
		}

		((uint8_t *) buffer)[temp] =
		  hex_to_decimal_convert (block_size_str);
	}

	/* Discard \r\n from the end */
	if (1 != read_buffered_port (fd, &discard, 1, ms)) {
		return -1;
	}

	if (1 != read_buffered_port (fd, &discard, 1, ms)) {
		return -1;
	}

	return result;
}

/* HEADER FUNCTIONS */

int
open_port (const char *port) {
	int result;

	if (NULL == port) {
		return -1;
	}

	result = open (port, O_RDWR);
	return result;
}

int
close_port (int fd) {
	int result;

	result = close (fd);
	return result;
}

int
open_socket (int fd, int ms) {
	const char open_command[19 + 1] = "AT+NSOCR=STREAM,6,0";
	int bytes_written;
	int bytes_read;
	char buffer[11];
	const char ok_response[6 + 1] = "\r\nOK\r\n";
	int result;

	if (fd < 0) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	bytes_written = write (fd, open_command, strlen (open_command));
	if (bytes_written < strlen (open_command)) {
		return -1;
	}

	bytes_written = write (fd, "\r\n", 2);
	if (bytes_written < 2) {
		return -1;
	}

	bytes_read = read_buffered_port (fd, buffer, 11, ms);
	if ((11 == bytes_read) &&
	    (0 ==
	     strncmp (buffer + bytes_read - strlen (ok_response), ok_response,
		      strlen (ok_response)))) {
		result = buffer[2];
	} else {
		/* DESTROY INPUT BUFFER ??? */
		result = -1;
	}

	return result;
}

int
connect_to_addr (int fd, int socket, const char *host, const char *port, int ms) {
	/* AT+NSOCO=[socket],[ipaddr],[port] */
	const char connect_command[9 + 1] = "AT+NSOCO=";
	int bytes_written;
	char buffer[9];
	int divider;
	int idx;
	int bytes_read;
	const char ok_response[6 + 1] = "\r\nOK\r\n";

	if (fd < 0) {
		return -1;
	}

	if (NULL == host) {
		return -1;
	}

	if (NULL == port) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}


	bytes_written = write (fd, connect_command, strlen (connect_command));
	if (bytes_written < strlen (connect_command)) {
		return -1;
	}

	/* Write socket + comma */
	buffer[0] = socket;
	buffer[1] = ',';
	bytes_written = write (fd, buffer, 2);
	if (bytes_written < 2) {
		return -1;
	}

	/* Write host */
	bytes_written = write (fd, host, strlen (host));
	if (bytes_written < strlen (host)) {
		return -1;
	}

	/* Generate port string */
	buffer[0] = ',';
	strcpy (buffer + 1, port);
	idx = strlen (buffer);

	buffer[idx] = '\r';
	++idx;
	buffer[idx] = '\n';
	++idx;

	/* Write comma + port */
	bytes_written = write (fd, buffer, idx);
	if (bytes_written < idx) {
		return -1;
	}

	bytes_read = read_buffered_port (fd, buffer, 9, ms);
	if (bytes_read < strlen (ok_response)) {
		return -1;
	}

	if (0 !=
	    strncmp (buffer + bytes_read - strlen (ok_response), ok_response,
		     strlen (ok_response))) {
		/* DESTROY INPUT BUFFER ??? */
		return -1;
	}
	return 0;
}

int
close_socket (int fd, int socket, int ms) {
	/* AT+NSOCL=[socket] */
	const char close_command[9 + 1] = "AT+NSOCL=";
	int bytes_written;
	char buffer[9];
	int bytes_read;
	const char ok_response[6 + 1] = "\r\nOK\r\n";

	if (fd < 0) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	bytes_written = write (fd, close_command, strlen (close_command));
	if (bytes_written < strlen (close_command)) {
		return -1;
	}

	buffer[0] = socket;
	buffer[1] = '\r';
	buffer[2] = '\n';
	bytes_written = write (fd, buffer, 3);
	if (bytes_written < 3) {
		return -1;
	}

	bytes_read = read_buffered_port (fd, buffer, 9, ms);

	if (bytes_read < strlen (ok_response)) {
		return -1;
	}

	if (0 ==
	    strncmp (buffer + bytes_read - strlen (ok_response), ok_response,
		     strlen (ok_response))) {
		/* DESTROY INPUT BUFFER ??? */
		return -1;
	}

	return 0;
}

int64_t
write_data (int fd, int dest, void *buffer, uint32_t size, int ms) {
	const int max_msg_len = 1538;
	int cycles;
	int curr_cycle;
	int curr_length;

	/* AT+NSOSD=[socket(1)],[length(4)],[data](1538*2)\r\n */
	char msg_buffer[18 + 2 * max_msg_len];
	const char write_command[9 + 1] = "AT+NSOSD=";
	int idx;
	int divider;
	int temp;
	uint8_t curr_byte;

	const char hex_table[16] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F'
	};
	int bytes_written;
	int response_size;
	int bytes_read;
	const char ok_response[6 + 1] = "\r\nOK\r\n";

	if (fd < 0) {
		return -1;
	}

	if (NULL == buffer) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	/* Calculate needed times to send to module */
	cycles = (size / max_msg_len) + 1;
	for (curr_cycle = 0; curr_cycle < cycles; ++curr_cycle) {
		if (curr_cycle == (cycles - 1)) {
			curr_length = size % max_msg_len;
		} else {
			curr_length = max_msg_len;
		}

		strcpy (msg_buffer, write_command);
		idx = strlen (msg_buffer);
		msg_buffer[idx] = dest;
		++idx;
		msg_buffer[idx] = ',';
		++idx;

		/* Stringify number */
		if (curr_length > 0) {
			divider = 1000;
			while (0 == (curr_length / divider)) {
				divider /= 10;
			}
			temp = curr_length;

			while (0 != divider) {
				msg_buffer[idx] = (temp / divider) + '0';
				++idx;
				temp = temp % divider;
				divider /= 10;
			}
		} else {
			msg_buffer[idx] = '0';
			++idx;
		}

		msg_buffer[idx] = ',';
		++idx;

		/* Turn data into HEX */
		for (temp = 0; temp < curr_length; ++temp) {
			curr_byte =
			  ((uint8_t *) buffer)[(curr_cycle * max_msg_len) +
					       temp];

			msg_buffer[idx] = hex_table[curr_byte >> 4];
			++idx;
			msg_buffer[idx] = hex_table[curr_byte & 0x0F];
			++idx;
		}
		msg_buffer[idx] = '\r';
		++idx;
		msg_buffer[idx] = '\n';
		++idx;

		bytes_written = write (fd, msg_buffer, idx);
		if (bytes_written < idx) {
			return -1;
		}

		/*
		 * \r\n[socket],[length]\r\n
		 * \r\nOK\r\n
		 */
		response_size = 12;
		temp = 1000;
		while (0 != temp) {
			if (0 != (curr_length / temp)) {
				++response_size;
			}
			temp /= 10;
		}
		bytes_read =
		  read_buffered_port (fd, msg_buffer, response_size, ms);

		if (bytes_read < strlen (ok_response)) {
			return -1;
		}

		if (0 !=
		    strncmp (msg_buffer + bytes_read - strlen (ok_response),
			     ok_response, strlen (ok_response))) {
			/* DESTROY INPUT BUFFER ??? */
			return -1;
		}
	}

	return size;
}

/* CONTAINS STATIC VARS !!! */
int64_t
read_data (int fd, int src, void *buffer, uint32_t size, int ms) {
	static char msg_buffer[1460];
	static int msg_size = 0;
	static int msg_idx = 0;
	const int max_msg_size = 1460;
	int64_t result;
	bool needs_more_blocks;
	int buff_idx;
	int copy_count;
	int i;

	if (fd < 0) {
		return -1;
	}

	if (NULL == buffer) {
		return -1;
	}

	if (ms <= 0) {
		ms = DEFAULT_TIMEOUT;
	}

	result = 0;
	needs_more_blocks = true;
	buff_idx = 0;
	while (needs_more_blocks) {
		if (msg_idx == msg_size) {
			msg_size = read_next_block (fd, msg_buffer, ms);
			msg_idx = 0;
			if (msg_size > max_msg_size) {
				return -1;
			}
		}

		if (msg_size < 0) {
			return -1;
		} else if (0 == msg_size) {
			break;
		}

		if (size <= result + msg_size - msg_idx) {
			copy_count = size - result;
			needs_more_blocks = false;
		} else {
			copy_count = msg_size - msg_idx;
		}

		for (i = 0; i < copy_count; ++i) {
			((uint8_t *) buffer)[buff_idx] = msg_buffer[msg_idx];
			++msg_idx;
			++buff_idx;
		}
		result += copy_count;
	}
	return result;
}
