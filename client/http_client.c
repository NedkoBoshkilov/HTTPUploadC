#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "comms.h"
#include "http_funcs.h"
#include "http_client.h"

/*
 * These define the type of generated boundary.
 * With BOUNDARY_DASH_COUNT = 8
 * and BOUNDARY_HEX_COUNT = 16
 * it looks like this:
 * --------0123456789ABCDEF
 */
#define BOUNDARY_DASH_COUNT 16
#define BOUNDARY_HEX_COUNT 16

/* Defines the size of the chunks for transfering the file */
#define BUFFER_SIZE 1024

/* Defines the maximum size of the block read buffers */
#define MAX_BLOCK_SIZE 1460

/* 
 * TODO: FIND A PROPER DEFINITION
 * Max length of a filename
 */
#define MAX_FILENAME_LENGTH 255

/* The following 3 variables hold the parameters for the RNG */
static uint32_t current_number;
static uint32_t multiplicative_part;
static uint32_t additive_part;

/* Content-Type enum */
enum {
	SHTTP_CONTTYPE_APP_URLENCODED = 0,
	SHTTP_CONTTYPE_TEXT_PLAIN,
	SHTTP_CONTTYPE_APP_OCTETSTREAM,
	SHTTP_CONTTYPE_MULTIPART_FORMDATA
};

/* PROTOTYPES */
/* Common functions */
static int build_full_host (char *_full_host, const char *_host,
			    const char *_port);

/* POST functions */
static int generate_boundary (char *_dest);
static uint32_t get_content_header_size (uint32_t _filename_length,
					 uint32_t _form_name_length,
					 int _content_type);
static uint32_t get_content_trailer_size (int _content_type);
static int64_t get_file_size (int _fd);
static int get_filename_from_path (char *_filename, const char *_filepath);
static int generate_and_send_post_request (int _fd_com_port, int _socket,
					   const char *_host,
					   uint32_t _host_length,
					   const char *_port, const char *_uri,
					   uint32_t _uri_length,
					   uint8_t _version,
					   const char *_boundary,
					   int _content_type,
					   uint32_t _content_length,
					   int _com_ms);
static int generate_and_send_post_content_header (int _fd_com_port, int _socket,
						  const char *_filename,
						  uint8_t _filename_length,
						  const char *_form_name,
						  uint8_t _form_name_length,
						  const char *_boundary,
						  int _content_type,
						  int _com_ms);
static int generate_and_send_post_content_trailer (int _fd_com_port,
						   int _socket,
						   const char *_boundary,
						   int _content_type,
						   int _com_ms);
static int send_file_data (int _fd_com_port, int _fd, int _socket, int _com_ms);
static int receive_and_decode_post (int _fd_com_port, int _socket,
				    char *_location_value, int _http_ms);

/* GET functions */
static int generate_and_send_get_request (int _fd_com_port, int _socket,
					  const char *_host,
					  uint32_t _host_length,
					  const char *_port, const char *_uri,
					  uint32_t _uri_length,
					  uint8_t _version, int _com_ms);
static int receive_and_decode_get (int _fd_com_port, int _socket,
				   const char *_filepath, char *_location_value,
				   uint32_t *_filesize, int _http_ms);
static int save_file (int _fd_com_port, int _socket, const char *_filepath,
		      uint32_t _filesize, int _http_ms, char *_msg_buffer,
		      int *_msg_size, int *_msg_idx, const int _max_msg_size);

/* DEFINITIONS */
static int
build_full_host (char *full_host, const char *host, const char *port) {
	int idx;
	int divisor;

	if (NULL == full_host) {
		return -1;
	}

	if (NULL == host) {
		return -1;
	}

	/* Copy Host */
	strcpy (full_host, host);
	idx = strlen (full_host);

	full_host[idx] = ':';
	++idx;
	full_host[idx] = '\0';

	/* Put port in */
	/*
	   divisor = 10000;
	   while (0 == (port / divisor))
	   {
	   divisor /= 10;
	   }

	   while (0 != divisor)
	   {
	   full_host[idx] = '0' + port / divisor;
	   ++idx;
	   port = port % divisor;
	   divisor /= 10;
	   }
	   full_host[idx] = '\0';
	 */
	strcat (full_host, port);

	return 0;
}

static int
generate_boundary (char *dest) {
	const char dash = '-';
	int idx;
	const int full_rounds = BOUNDARY_HEX_COUNT / 8;
	const int round_filler = BOUNDARY_HEX_COUNT % 8;
	int compare;
	int i;
	uint32_t random_number;
	uint32_t mask = 0x0000000F;

	const char hex[] = {
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F',
	};

	if (NULL == dest) {
		return -1;
	}

	/* Add dashes */
	for (idx = 0; idx < BOUNDARY_DASH_COUNT; ++idx) {
		dest[idx] = dash;
	}

	/* Add random hex symbols */
	for (idx = 0; idx < full_rounds + 1; ++idx) {
		if (idx < full_rounds) {
			compare = 8;
		} else {
			compare = round_filler;
		}
		random_number = generate_number ();

		for (i = 0; i < compare; ++i) {
			dest[idx * 8 + i + BOUNDARY_DASH_COUNT] =
			  hex[random_number & mask];
			random_number >>= 4;
		}
	}

	/* NULL termination */
	dest[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT] = '\0';

	return 0;
}

static uint32_t
get_content_header_size (uint32_t filename_length, uint32_t form_name_length,
			 int content_type) {
	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		return 100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT +
		  filename_length + form_name_length;
	}
	return 0;
}

static uint32_t
get_content_trailer_size (int content_type) {
	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		return 8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT;
	}

	return 0;
}

static int64_t
get_file_size (int fd) {
	int64_t result = 0;

	result = lseek (fd, 0, SEEK_END);

	if (0 != result) {
		if (0 != lseek (fd, 0, SEEK_SET)) {
			result = -1;
		}
	}

	return result;
}

static int
get_filename_from_path (char *filename, const char *filepath) {
	int start;
	int idx;
	const char delimiters[2] = { '/', '\\' };

	if (NULL == filename) {
		return -1;
	}

	if (NULL == filepath) {
		return -1;
	}

	/* Find the starting position of the filename */
	start = 0;
	for (idx = 0; filepath[idx] != '\0'; ++idx) {
		if ((filepath[idx] == delimiters[0]) ||
		    (filepath[idx] == delimiters[1])) {
			start = idx + 1;
		}
	}

	strcpy (filename, &(filepath[start]));

	return 0;
}

static int
generate_and_send_post_request (int fd_com_port, int socket, const char *host,
				uint32_t host_length, const char *port,
				const char *uri, uint32_t uri_length,
				uint8_t version, const char *boundary,
				int content_type, uint32_t content_length,
				int com_ms) {
	char full_host[host_length + 6];
	char referer_name[8];
	char referer_value[21 + uri_length + 1];
	char type_name[13];
	char type_value[30 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	char length_name[15];
	char length_value[11];	/* 2^32 is 10 Digits */
	uint32_t divisor;
	int idx;
	char conn_name[11];
	char conn_value[11];
	struct http_header headers[4];
	struct http_request request_params;
	char request[159 + 2 * uri_length + BOUNDARY_DASH_COUNT +
		     BOUNDARY_HEX_COUNT + 1];
	uint32_t request_size;
	int result;

	/*
	 * POST [uri] HTTP/1.X\r\n
	 * Host:[full_host]\r\n
	 * Referer:[full_host + uri]\r\n
	 * --- Content-Type:multipart/form-data; boundary=[boundary]\r\n
	 * --- Content-Type:text/plain\r\n
	 * --- Content-Type:application/octet-stream\r\n
	 * Content-Length:[content_length]\r\n
	 * Connection:[keep-alive|close]\r\n
	 * \r\n
	 */

	if (NULL == host) {
		return -1;
	}

	if (NULL == uri) {
		return -1;
	}

	if (NULL == boundary) {
		return -1;
	}

	/* Create full host */
	if (0 != build_full_host (full_host, host, port)) {
		return -1;
	}

	/* Fill referer header */
	strcpy (referer_name, "Referer");
	strcpy (referer_value, full_host);

	if (NULL == strchr (uri, '?')) {
		strcat (referer_value, uri);
	} else {
		strncat (referer_value, uri, strchr (uri, '?') - uri);
	}

	/* Fill content-type header */
	strcpy (type_name, "Content-Type");
	switch (content_type) {
	case SHTTP_CONTTYPE_TEXT_PLAIN:
		strcpy (type_value, "text/plain");
		break;

	case SHTTP_CONTTYPE_APP_OCTETSTREAM:
		strcpy (type_value, "application/octet-stream");
		break;

	case SHTTP_CONTTYPE_MULTIPART_FORMDATA:
		strcpy (type_value, "multipart/form-data; boundary=");
		strcat (type_value, boundary);
		break;

	default:
		return -1;
	}

	/* Fill content-length header */
	strcpy (length_name, "Content-Length");

	divisor = 1000000000;
	idx = 0;
	while (0 == (content_length / divisor)) {
		divisor /= 10;
	}

	while (0 != divisor) {
		length_value[idx] = '0' + content_length / divisor;
		++idx;
		content_length = content_length % divisor;
		divisor /= 10;
	}
	length_value[idx] = '\0';

	/* Fill connection header */
	strcpy (conn_name, "Connection");
	/* strcpy(conn_value, "keep-alive"); */
	strcpy (conn_value, "close");

	/* Fill header structures */
	headers[0].name = referer_name;
	headers[0].value = referer_value;
	headers[1].name = type_name;
	headers[1].value = type_value;
	headers[2].name = length_name;
	headers[2].value = length_value;
	headers[3].name = conn_name;
	headers[3].value = conn_value;

	/* Fill request structure */
	strcpy (request_params.method, "POST");
	request_params.uri = uri;
	request_params.is_version11 = version;
	request_params.host = full_host;
	request_params.header_count = 4;
	request_params.headers = headers;

	/* Build and send request */
	result = build_request (&request_params, request, &request_size);
	if (0 != result) {
		return -1;
	}

	result =
	  write_data (fd_com_port, socket, request, request_size, com_ms);
	if (request_size == result) {
		result = 0;
	}

	return result;
}

static int
generate_and_send_post_content_header (int fd_com_port, int socket,
				       const char *filename,
				       uint8_t filename_length,
				       const char *form_name,
				       uint8_t form_name_length,
				       const char *boundary, int content_type,
				       int com_ms) {
	const int header_size =
	  100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + filename_length +
	  form_name_length + 1;
	char header[header_size];
	int result = 0;

	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA != content_type) {
		return 0;
	}

	if (NULL == filename) {
		return -1;
	}

	if (NULL == form_name) {
		return -1;
	}

	if (NULL == boundary) {
		return -1;
	}

	result =
	  build_upload_payload_header (filename, form_name, boundary, header,
				       NULL);
	if (0 != result) {
		return -1;
	}

	result =
	  write_data (fd_com_port, socket, header, header_size - 1, com_ms);
	if (header_size - 1 == result) {
		result = 0;
	}

	return result;
}

static int
generate_and_send_post_content_trailer (int fd_com_port, int socket,
					const char *boundary, int content_type,
					int com_ms) {
	const int trailer_size =
	  8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1;
	char trailer[trailer_size];
	int result;

	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA != content_type) {
		return 0;
	}

	if (NULL == boundary) {
		return -1;
	}

	result = build_upload_payload_trailer (boundary, trailer, NULL);
	if (0 != result) {
		return -1;
	}

	result =
	  write_data (fd_com_port, socket, trailer, trailer_size - 1, com_ms);
	if (trailer_size - 1 == result) {
		result = 0;
	}

	return result;
}

static int
send_file_data (int fd_com_port, int fd, int socket, int com_ms) {
	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_written;
	int result;

	result = 0;
	bytes_read = read (fd, buffer, BUFFER_SIZE);
	while (bytes_read > 0) {
		bytes_written =
		  write_data (fd_com_port, socket, buffer, bytes_read, com_ms);

		if (bytes_written != bytes_read) {
			result = -1;
			break;
		}

		bytes_read = read (fd, buffer, BUFFER_SIZE);
	}

	if (bytes_read < 0) {
		result = -1;
	}

	return result;
}

static int
receive_and_decode_post (int fd_com_port, int socket, char *location_value,
			 int http_ms) {
	/*
	 * HTTP/1.X YYY [response_text]\r\n
	 * Header: Value\r\n ...
	 * \r\n
	 */

	uint16_t buff_idx;
	uint8_t buff[BUFFER_SIZE + 1];
	char msg_buffer[MAX_BLOCK_SIZE];
	int msg_size = 0;
	int msg_idx = 0;
	int result;
	char length_name[15];
	char length_value[17];
	char location_name[9];
	struct http_header headers[2];
	struct http_response response;
	uint32_t discard_size;
	int current_read_size;

	buff_idx = 0;
	while ((buff_idx < 4) ||
	       (0 != strncmp (&(buff[buff_idx - 4]), "\r\n\r\n", 4)) &&
	       (buff_idx < BUFFER_SIZE)) {
		result =
		  read_data (fd_com_port, socket, buff + buff_idx, 1, http_ms,
			     msg_buffer, &msg_size, &msg_idx, MAX_BLOCK_SIZE);
		if (result <= 0) {
			break;
		}
		++buff_idx;
	}
	buff[buff_idx] = '\0';

	if (buff_idx < 13) {
		return -1;
	}

	if (0 != strncmp (&(buff[buff_idx - 4]), "\r\n\r\n", 4)) {
		if (buff_idx + 3 > BUFFER_SIZE) {
			/* -- */
			buff[buff_idx] = '\0';
			buff[buff_idx - 1] = '\n';
			buff[buff_idx - 2] = '\r';
			buff[buff_idx - 3] = '\n';
			buff[buff_idx - 4] = '\r';
		} else {
			/* ++ */
			buff[buff_idx] = '\r';
			buff[buff_idx + 1] = '\n';
			buff[buff_idx + 2] = '\r';
			buff[buff_idx + 3] = '\n';
			buff[buff_idx + 4] = '\0';
		}
	}

	/* Fill header name and set to search for it */
	strcpy (length_name, "Content-Length");
	length_value[0] = '\0';
	strcpy (location_name, "Location");
	location_value[0] = '\0';
	headers[0].name = length_name;
	headers[0].value = length_value;
	headers[1].name = location_name;
	headers[1].value = location_value;
	response.header_count = 2;
	response.headers = headers;

	result = decode_response (buff, &response, NULL);
	if (0 == result) {
		if (2 != (response.code / 100)) {
			result = response.code;
		} else {
			/* Find how many bytes we need to read and discard */
			discard_size = 0;
			buff_idx = 0;
			for (buff_idx = 0; length_value[buff_idx] != '\0';
			     ++buff_idx) {
				discard_size *= 10;
				discard_size += length_value[buff_idx] - '0';
			}

			current_read_size = BUFFER_SIZE;
			while (discard_size > 0) {
				if (discard_size < BUFFER_SIZE) {
					current_read_size = discard_size;
				}

				result =
				  read_data (fd_com_port, socket, buff,
					     current_read_size, http_ms,
					     msg_buffer, &msg_size, &msg_idx,
					     MAX_BLOCK_SIZE);
				if (result < 0) {
					/* MAYBE RETURN -1 ??? FAILED DISCARD OF UNNEEDED DATA ??? */
					break;
				}
				discard_size -= result;
			}
			result = 0;
		}
	}
	return result;
}

static int
generate_and_send_get_request (int fd_com_port, int socket, const char *host,
			       uint32_t host_length, const char *port,
			       const char *uri, uint32_t uri_length,
			       uint8_t version, int com_ms) {
	char full_host[22];
	char conn_name[11];
	char conn_value[11];
	struct http_header headers[1];
	struct http_request request_params;
	char request[69 + uri_length];
	uint32_t request_size;
	int result;

	/*
	 * GET [uri] HTTP/1.X\r\n
	 * Host:[full_host]\r\n
	 * Connection:[keep-alive|close]\r\n
	 * \r\n
	 */

	if (NULL == host) {
		return -1;
	}

	if (NULL == uri) {
		return -1;
	}

	/* Build full host */
	if (0 != build_full_host (full_host, host, port)) {
		return -1;
	}

	/* Fill connection header */
	strcpy (conn_name, "Connection");
	/* strcpy(conn_value, "keep-alive"); */
	strcpy (conn_value, "close");

	/* Fill header structures */
	headers[0].name = conn_name;
	headers[0].value = conn_value;

	/* Fill request structure */
	strcpy (request_params.method, "GET");
	request_params.uri = uri;
	request_params.is_version11 = version;
	request_params.host = full_host;
	request_params.header_count = 1;
	request_params.headers = headers;

	/* Build and send request */
	result = build_request (&request_params, request, &request_size);
	if (0 != result) {
		return -1;
	}

	result =
	  write_data (fd_com_port, socket, request, request_size, com_ms);
	if (request_size == result) {
		result = 0;
	}

	return result;
}

static int
receive_and_decode_get (int fd_com_port, int socket, const char *filepath,
			char *location_value, uint32_t *filesize,
			int http_ms) {
	uint16_t buff_idx;
	uint8_t buff[BUFFER_SIZE + 1];
	char msg_buffer[MAX_BLOCK_SIZE];
	int msg_size = 0;
	int msg_idx = 0;
	int result;
	char length_name[15];
	char length_value[17];
	char location_name[9];
	struct http_header headers[2];
	struct http_response response;
	uint32_t filesize_local;

	/*
	 * HTTP/1.X YYY [response_text]\r\n
	 * Header: Value\r\n ...
	 * \r\n
	 */

	buff_idx = 0;
	while ((buff_idx < 4) ||
	       (0 != strncmp (&(buff[buff_idx - 4]), "\r\n\r\n", 4)) &&
	       (buff_idx < BUFFER_SIZE)) {
		result =
		  read_data (fd_com_port, socket, buff + buff_idx, 1, http_ms,
			     msg_buffer, &msg_size, &msg_idx, MAX_BLOCK_SIZE);
		if (result <= 0) {
			return -1;
		}
		++buff_idx;
	}
	buff[buff_idx] = '\0';

	if (0 != strncmp (&(buff[buff_idx - 4]), "\r\n\r\n", 4)) {
		return -1;
	}

	/* Fill header name and set to search for it */
	strcpy (length_name, "Content-Length");
	length_value[0] = '\0';

	strcpy (location_name, "Location");
	location_value[0] = '\0';

	headers[0].name = length_name;
	headers[0].value = length_value;

	headers[1].name = location_name;
	headers[1].value = location_value;

	response.header_count = 2;
	response.headers = headers;

	result = decode_response (buff, &response, NULL);
	if (0 == result) {
		if (2 != (response.code / 100)) {
			result = response.code;
		} else {
			filesize_local = 0;
			buff_idx = 0;
			for (buff_idx = 0; length_value[buff_idx] != '\0';
			     ++buff_idx) {
				filesize_local *= 10;
				filesize_local += length_value[buff_idx] - '0';
			}

			if (NULL != filesize) {
				*filesize = filesize_local;
			}

			result =
			  save_file (fd_com_port, socket, filepath,
				     filesize_local, http_ms, msg_buffer,
				     &msg_size, &msg_idx, MAX_BLOCK_SIZE);
		}
	}
	return result;
}

static int
save_file (int fd_com_port, int socket, const char *filepath, uint32_t filesize,
	   int http_ms, char *msg_buffer, int *msg_size, int *msg_idx,
	   const int max_msg_size) {
	int result;
	uint8_t name_idx;
	int fd;
	int to_read;
	uint8_t buffer[BUFFER_SIZE];
	int bytes_written;

	if (NULL == filepath) {
		return -1;
	}

	fd = open (filepath, O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG | S_IRWXO);
	if (fd < 0) {
		return -1;
	}

	result = 1;
	to_read = BUFFER_SIZE;
	while (result > 0 && (0 != filesize)) {
		if (filesize < BUFFER_SIZE) {
			to_read = filesize;
		}
		result =
		  read_data (fd_com_port, socket, buffer, to_read, http_ms,
			     msg_buffer, msg_size, msg_idx, max_msg_size);
		if (result > 0) {
			bytes_written = write (fd, buffer, result);
			if ((bytes_written != result) ||
			    (bytes_written > filesize)) {
				close (fd);
				return -1;
			}
			filesize -= bytes_written;
		}
	}
	if (result > 0) {
		result = 0;
	}
	close (fd);
	return result;
}

/* Header file function definitions */
int
upload_file (const char *com_port, const char *filepath, const char *form_name,
	     const char *host, const char *port, const char *uri,
	     char *next_location, int content_type, int com_ms, int http_ms) {
	int error;
	char boundary[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	int fd_com_port;
	char filename[MAX_FILENAME_LENGTH + 1];
	int file_descriptor;
	int64_t file_size;
	int socket_descriptor;

	if (NULL == com_port) {
		return -1;
	}

	if (NULL == filepath) {
		return -1;
	}

	if (NULL == host) {
		return -1;
	}

	if (NULL == port) {
		return -1;
	}

	if (NULL == uri) {
		return -1;
	}

	if (SHTTP_CONTTYPE_APP_URLENCODED == content_type) {
		return -1;
	}

	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		if (NULL == form_name) {
			return -1;
		}

		/* Preparations */
		error = generate_boundary (boundary);
		if (0 != error) {
			return -1;
		}
	}

	fd_com_port = open_port (com_port);
	if (fd_com_port < 0) {
		return -1;
	}

	/* Preparations */
	error = get_filename_from_path (filename, filepath);
	if (0 != error) {
		close_port (fd_com_port);
		return -1;
	}

	file_descriptor = open (filepath, O_RDONLY);
	if (file_descriptor < 0) {
		close_port (fd_com_port);
		return -1;
	}

	file_size = get_file_size (file_descriptor);
	if (file_size < 0) {
		close (file_descriptor);
		close_port (fd_com_port);
		return -1;
	}

	/* Connection */
	socket_descriptor = open_socket (fd_com_port, com_ms);
	if (socket_descriptor < 0) {
		close (file_descriptor);
		close_port (fd_com_port);
		return -1;
	}

	error =
	  connect_to_addr (fd_com_port, socket_descriptor, host, port, com_ms);
	if (0 != error) {
		close_socket (fd_com_port, socket_descriptor, com_ms);
		close (file_descriptor);
		return -1;
	}

	/* Data sending */
	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		error =
		  generate_and_send_post_request (fd_com_port,
						  socket_descriptor, host,
						  strlen (host), port, uri,
						  strlen (uri), 1, boundary,
						  content_type,
						  file_size +
						  get_content_header_size
						  (strlen (filename),
						   strlen (form_name),
						   content_type) +
						  get_content_trailer_size
						  (content_type), com_ms);
	} else {
		error =
		  generate_and_send_post_request (fd_com_port,
						  socket_descriptor, host,
						  strlen (host), port, uri,
						  strlen (uri), 1, boundary,
						  content_type, file_size,
						  com_ms);
	}

	if (0 != error) {
		close_socket (fd_com_port, socket_descriptor, com_ms);
		close (file_descriptor);
		close_port (fd_com_port);
		return -1;
	}

	/* Looking for response when we're redirected */
	error =
	  receive_and_decode_post (fd_com_port, socket_descriptor,
				   next_location, http_ms);
	if (-1 != error) {
		close_socket (fd_com_port, socket_descriptor, com_ms);
		close (file_descriptor);
		close_port (fd_com_port);
		return error;
	}

	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		error =
		  generate_and_send_post_content_header (fd_com_port,
							 socket_descriptor,
							 filename,
							 strlen (filename),
							 form_name,
							 strlen (form_name),
							 boundary, content_type,
							 com_ms);
		if (0 != error) {
			close_socket (fd_com_port, socket_descriptor, com_ms);
			close (file_descriptor);
			close_port (fd_com_port);
			return -1;
		}
	}

	error =
	  send_file_data (fd_com_port, file_descriptor, socket_descriptor,
			  com_ms);
	if (0 != error) {
		close_socket (fd_com_port, socket_descriptor, com_ms);
		close (file_descriptor);
		close_port (fd_com_port);
		return -1;
	}

	if (SHTTP_CONTTYPE_MULTIPART_FORMDATA == content_type) {
		error =
		  generate_and_send_post_content_trailer (fd_com_port,
							  socket_descriptor,
							  boundary,
							  content_type, com_ms);
		if (0 != error) {
			close_socket (fd_com_port, socket_descriptor, com_ms);
			close (file_descriptor);
			close_port (fd_com_port);
			return -1;
		}
	}

	/* Response Receiving */
	error =
	  receive_and_decode_post (fd_com_port, socket_descriptor,
				   next_location, http_ms);

	/* Cleanup */
	close_socket (fd_com_port, socket_descriptor, com_ms);
	close (file_descriptor);
	close_port (fd_com_port);

	return error;
}

int
get_resource (const char *com_port, const char *filepath, uint32_t *filesize,
	      const char *host, const char *port, const char *uri,
	      char *next_location, int com_ms, int http_ms) {
	int fd_com_port;
	int socket;
	int result;

	if (NULL == com_port) {
		return -1;
	}

	if (NULL == filepath) {
		return -1;
	}
	if (NULL == host) {
		return -1;
	}
	if (NULL == port) {
		return -1;
	}
	if (NULL == uri) {
		return -1;
	}
	if (NULL == next_location) {
		return -1;
	}

	fd_com_port = open_port (com_port);
	if (fd_com_port < 0) {
		return -1;
	}

	/* Connection */
	socket = open_socket (fd_com_port, com_ms);
	if (socket < 0) {
		close_port (fd_com_port);
		return -1;
	}

	result = connect_to_addr (fd_com_port, socket, host, port, com_ms);
	if (0 != result) {
		close_socket (fd_com_port, socket, com_ms);
		close_port (fd_com_port);
		return -1;
	}

	/* Data sending */
	result =
	  generate_and_send_get_request (fd_com_port, socket, host,
					 strlen (host), port, uri, strlen (uri),
					 1, com_ms);
	if (0 != result) {
		close_socket (fd_com_port, socket, com_ms);
		close_port (fd_com_port);
		return -1;
	}

	/* Response Receiving */
	result =
	  receive_and_decode_get (fd_com_port, socket, filepath, next_location,
				  filesize, http_ms);

	/* Cleanup */
	close_socket (fd_com_port, socket, com_ms);
	close_port (fd_com_port);
	return result;
}

void
set_seed (uint32_t start_value, uint32_t mult_part, uint32_t add_part) {
	uint8_t fixer;

	current_number = start_value;
	fixer = (mult_part - 1) % 4;
	if (fixer != 0) {
		mult_part -= fixer;
	}
	multiplicative_part = mult_part;

	fixer = add_part % 2;
	if (fixer != 1) {
		add_part += 1;
	}
	additive_part = add_part;
}

uint32_t
generate_number () {
	current_number = (current_number * multiplicative_part) + additive_part;
	return current_number;
}
