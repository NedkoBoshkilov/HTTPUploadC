#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "comms.h"
#include "http_funcs.h"
#include "http_uploader.h"

/*
 * These define the type of generated boundary.
 * With BOUNDARY_DASH_COUNT = 8
 * and BOUNDARY_HEX_COUNT = 16
 * it looks like this:
 * --------0123456789ABCDEF
 */
#define BOUNDARY_DASH_COUNT 16
#define BOUNDARY_HEX_COUNT 16

/* Defines the size of the chunks for transfering the file*/
#define BUFFER_SIZE 1024

/* 
 * TODO: FIND A PROPER DEFINITION
 * Max length of a filename
 */
#define MAX_FILENAME_LENGTH 255

/* The following 3 variables hold the parameters for the RNG */
static uint32_t current_number;
static uint32_t multiplicative_part;
static uint32_t additive_part;

static uint32_t
get_content_header_size(uint32_t filename_length, uint32_t form_name_length)
{
	return 100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + filename_length + form_name_length;
}

static uint32_t
get_content_trailer_size()
{
	return 8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT;
}

static int
generate_boundary(char *dest)
{
	const char dash = '-';
	int idx;
	const int full_rounds = BOUNDARY_HEX_COUNT / 8;
	const int round_filler = BOUNDARY_HEX_COUNT % 8;
	int compare;
	int i;
	uint32_t random_number;
	uint32_t mask = 0x0000000F;

	const char hex[] =
	{
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F',
	};

	if (NULL == dest)
	{
		return -1;
	}
	
	/* Add dashes */
	for (idx = 0; idx < BOUNDARY_DASH_COUNT; ++idx)
	{
		dest[idx] = dash;
	}
	
	/* Add random hex symbols */
	for (idx = 0; idx < full_rounds + 1; ++idx)
	{
		if (idx < full_rounds)
		{
			compare = 8;
		}
		else
		{
			compare = round_filler;
		}
		random_number = generate_number();

		for (i = 0; i < compare; ++i)
		{
			dest[idx * 8 + i + BOUNDARY_DASH_COUNT] = hex[random_number & mask];
			random_number >>= 4;
		}
	}
	
	/* NULL termination */
	dest[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT] = '\0';

	return 0;
}

static int
get_filename_from_path(char *filename, const char *filepath)
{
	int start;
	int idx;
	const char delimiters[2] = {'/', '\\'};

	if (NULL == filename)
	{
		return -1;
	}

	if (NULL == filepath)
	{
		return -1;
	}

	/* Find the starting position of the filename */
	start = 0;
	for (idx = 0; filepath[idx] != '\0'; ++idx)
	{
		if( (filepath[idx] == delimiters[0]) || (filepath[idx] == delimiters[1]) )
		{
			start = idx + 1;
		}
	}

	strcpy(filename, &(filepath[start]));

	return 0;
}

static int
get_file_size(int fd)
{
	int result = 0;

	result = lseek(fd, 0, SEEK_END);

	if (0 != result)
	{
		if (0 != lseek(fd, 0, SEEK_SET))
		{
			result = -1;
		}
	}

	return result;
}

static int
build_full_host(char *full_host, const char *host, uint16_t port)
{
	int idx;
	int divisor;

	if (NULL == full_host)
	{
		return -1;
	}

	if (NULL == host)
	{
		return -1;
	}

	/* Copy Host */
	strcpy(full_host, host);
	idx = strlen(full_host);

	full_host[idx] = ':';
	++idx;

	/* Put port in */
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

	return 0;
}

static int
generate_and_send_request(int socket, const char *host, uint16_t port, const char *uri, uint32_t uri_length, uint8_t version, const char *boundary, uint32_t content_length)
{
	char full_host[22];
	char referer_name[8];
	char referer_value[21 + uri_length + 1];
	char type_name[13];
	char type_value[30 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	char length_name[15];
	char length_value[11]; /* 2^32 is 10 Digits */
	uint32_t divisor;
	int idx;
	char conn_name[11];
	char conn_value[11];
	struct http_header headers[4];
	struct http_request request_params;
	char request[159 + 2 * uri_length + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	uint32_t request_size;
	int result;

	if (NULL == host)
	{
		return -1;
	}

	if (NULL == uri)
	{
		return -1;
	}

	if (NULL == boundary)
	{
		return -1;
	}

	/* Create fullHost */
	if (0 != build_full_host(full_host, host, port))
	{
		return -1;
	}

	/* Fill referer header */
	strcpy(referer_name, "Referer");
	strcpy(referer_value, full_host);
	strcat(referer_value, uri);

	/* Fill content-type header */
	strcpy(type_name, "Content-Type");
	strcpy(type_value, "multipart/form-data; boundary=");
	strcat(type_value, boundary);

	/* Fill content-length header */
	strcpy(length_name, "Content-Length");

	divisor = 1000000000;
	idx = 0;
	while (0 == (content_length / divisor))
	{
		divisor /= 10;
	}

	while (0 != divisor)
	{
		length_value[idx] = '0' + content_length / divisor;
		++idx;
		content_length = content_length % divisor;
		divisor /= 10;
	}
	length_value[idx] = '\0';

	strcpy(conn_name, "Connection");
	strcpy(conn_value, "keep-alive");

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
	strcpy(request_params.method, "POST");
	request_params.uri = uri;
	request_params.is_version11 = version;
	request_params.host = full_host;
	request_params.header_count = 4;
	request_params.headers = headers;

	/* Build and send request */
	result = build_request(&request_params, request, &request_size);
	if (0 != result)
	{
		return -1;
	}

	result = write_data(socket, request, request_size);
	if (request_size == result)
	{
		result = 0;
	}

	return result;
}

static int
generate_and_send_content_header(int socket, const char *filename, uint8_t filename_length, const char *form_name, uint8_t form_name_length, const char *boundary)
{
	const int header_size = 100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + filename_length + form_name_length + 1;
	char header[header_size];
	int result = 0;

	if (NULL == filename)
	{
		return -1;
	}

	if (NULL == form_name)
	{
		return -1;
	}

	if (NULL == boundary)
	{
		return -1;
	}

	result = build_upload_payload_header(filename, form_name, boundary, header, NULL);
	if (0 != result)
	{
		return -1;
	}

	result = write_data(socket, header, header_size - 1);
	if (header_size - 1 == result)
	{
		result = 0;
	}

	return result;
}

static int
send_file_data(int fd, int socket)
{
	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_written;
	int result;

	result = 0;
	bytes_read = read(fd, buffer, BUFFER_SIZE);
	while (bytes_read > 0)
	{
		bytes_written = write_data(socket, buffer, bytes_read);

		if (bytes_written != bytes_read)
		{
			result = -1;
			break;
		}

		bytes_read = read(fd, buffer, BUFFER_SIZE);
	}

	if (bytes_read < 0)
	{
		result = -1;
	}

	return result;
}

static int
generate_and_send_content_trailer(int socket, const char *boundary)
{
	const int trailer_size = 8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1;
	char trailer[trailer_size];
	int result;

	if (NULL == boundary)
	{
		return -1;
	}

	result = build_upload_payload_trailer(boundary, trailer, NULL);
	if (0 != result)
	{
		return -1;
	}

	result = write_data(socket, trailer, trailer_size - 1);
	if (trailer_size - 1 == result)
	{
		result = 0;
	}

	return result;
}

static int
receive_and_decode(int socket)
{
	
	struct http_response response;
	uint8_t bytes_read;
	uint8_t buff[18];
	int result;
	char the_void;

	response.header_count = 0;
	bytes_read = 0;

	/* Until data apears on the socket */
	while (0 == bytes_read)
	{
		bytes_read = read_data(socket, buff, 13);
	}

	if (bytes_read < 0)
	{
		return -1;
	}

	/* Until relevant info is read */
	while (bytes_read != 13)
	{
		result = read_data(socket, buff + bytes_read, 13 - bytes_read);
		if (result < 0)
		{
			return -1;
		}
		bytes_read += result;
	}

	/* Discard unneeded data */
	while (result = read_data(socket, &the_void, 1))
	{
		if (result < 0)
		{
			return -1;
		}
	}

	/* Put nessesary symbols for the parser */
	buff[13] = '\r';
	buff[14] = '\n';
	buff[15] = '\r';
	buff[16] = '\n';
	buff[17] = '\0';

	result = decode_response(buff, &response, NULL);

	if (0 != result)
	{
		return result;
	}

	if (2 != (response.code / 100))
	{
		result = response.code;
	}

	return result;
}

int
upload_file(const char *filepath, const char *form_name, const char *host, uint16_t port, const char *uri)
{
	int error;
	char boundary[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	char filename[MAX_FILENAME_LENGTH + 1];
	int file_descriptor;
	int file_size;
	int socket_descriptor;

	if (NULL == filepath)
	{
		return -1;
	}

	if (NULL == form_name)
	{
		return -1;
	}

	if (NULL == host)
	{
		return -1;
	}

	if (NULL == uri)
	{
		return -1;
	}

	/* Preparations */
	error = generate_boundary(boundary);
	if (0 != error)
	{
		return -1;
	}

	error = get_filename_from_path(filename, filepath);
	if (0 != error)
	{
		return -1;
	}
	
	file_descriptor = open(filepath, O_RDONLY);
	if (file_descriptor < 0)
	{
		return -1;
	}

	file_size = get_file_size(file_descriptor);
	if (file_size < 0)
	{
		close(file_descriptor);
		return -1;
	}

	/* Connection */
	socket_descriptor = open_socket();
	if (socket_descriptor < 0)
	{
		close(file_descriptor);
		return -1;
	}

	error = connect_to_addr(socket_descriptor, host, port);
	if (0 != error)
	{
		close_socket(socket_descriptor);
		close(file_descriptor);
		return -1;
	}

	/* Data sending */
	error = generate_and_send_request(socket_descriptor, host, port, uri, strlen(uri), 1, boundary,
		file_size + get_content_header_size(strlen(filename), strlen(form_name)) + get_content_trailer_size());
	if (0 != error)
	{
		close_socket(socket_descriptor);
		close(file_descriptor);
		return -1;
	}
	
	error = generate_and_send_content_header(socket_descriptor, filename, strlen(filename), form_name, strlen(form_name), boundary);
	if (0 != error)
	{
		close_socket(socket_descriptor);
		close(file_descriptor);
		return -1;
	}

	error = send_file_data(file_descriptor, socket_descriptor);
	if (0 != error)
	{
		close_socket(socket_descriptor);
		close(file_descriptor);
		return -1;
	}

	error = generate_and_send_content_trailer(socket_descriptor, boundary);
	if (0 != error)
	{
		close_socket(socket_descriptor);
		close(file_descriptor);
		return -1;
	}

	/* Response Receiving */
	error = receive_and_decode(socket_descriptor);

	/* Cleanup */
	close_socket(socket_descriptor);
	close(file_descriptor);

	return error;
}

void
set_seed(uint32_t start_value, uint32_t mult_part, uint32_t add_part)
{
	uint8_t fixer;

	current_number = start_value;
	fixer = (mult_part - 1) % 4;
	if (fixer != 0)
	{
		mult_part -= fixer;
	}
	multiplicative_part = mult_part;

	fixer = add_part % 2;
	if (fixer != 1)
	{
		add_part += 1;
	}
	additive_part = add_part;
}

uint32_t
generate_number()
{
	current_number = (current_number * multiplicative_part) + additive_part;
	return current_number;
}
