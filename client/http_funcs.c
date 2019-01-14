#include <string.h>

#include "http_funcs.h"

static const char line_delimiter[] = "\r\n";
static const char http_version_text[] = "HTTP/1.";
static const char host_header_name[] = "Host";
static const char header_name_value_delimiter = ':';
static const char boundary_delimiter[] = "--";
static const char content_header_part1[] =
  "Content-Disposition: form-data; name=\"";
static const char content_header_part2[] = "\"; filename=\"";
static const char content_header_part3[] =
  "Content-Type: application/octet-stream";

static bool
is_whitespace (const char test) {
	bool result;

	if ((' ' == test) || ('\t' == test)) {
		result = true;
	} else {
		result = false;
	}

	return result;
}

static uint32_t
next_line_start (const char *buff, uint32_t start) {
	uint32_t result;
	bool found;

	if (NULL == buff) {
		return start;
	}

	result = start;
	found = false;
	while (!found) {
		if ((line_delimiter[0] == buff[result - 1]) &&
		    (line_delimiter[1] == buff[result])) {
			found = true;
		}
		++result;
	}
	return result;
}

static uint32_t
next_delimiter (const char *buff, uint32_t start) {
	uint32_t result;
	bool found;

	if (NULL == buff) {
		return start;
	}

	result = start;
	found = false;
	while (!found) {
		if (header_name_value_delimiter == buff[result]) {
			found = true;
		}
		++result;
	}
	return result - 1;
}

int
build_request (const struct http_request *request, char *dest,
	       uint32_t *request_size) {
	/*
	 * Format of the request:
	 * [method] [uri] HTTP/1.X\r\n
	 * Host:[host]\r\n
	 * [Name]:[value]\r\n
	 * \r\n
	 */

	struct http_header current_header;
	uint8_t idx_header;

	if (NULL == request) {
		return -1;
	}

	if (NULL == dest) {
		return -1;
	}

	if (NULL == request->uri) {
		return -1;
	}

	if (NULL == request->host) {
		return -1;
	}

	/* Set first line */
	strcpy (dest, request->method);
	strcat (dest, " ");
	strcat (dest, request->uri);
	strcat (dest, " ");
	strcat (dest, http_version_text);

	if (request->is_version11) {
		strcat (dest, "1");
	} else {
		strcat (dest, "0");
	}
	strcat (dest, line_delimiter);

	/* Set host header */
	strcat (dest, host_header_name);

	dest[strlen (dest) + 1] = '\0';
	dest[strlen (dest)] = header_name_value_delimiter;

	strcat (dest, request->host);
	strcat (dest, line_delimiter);

	/* Set other headers */
	for (idx_header = 0; idx_header < request->header_count; ++idx_header) {
		if (NULL == request->headers) {
			return -1;
		}

		current_header = request->headers[idx_header];

		if ((NULL == current_header.name) ||
		    (NULL == current_header.value)) {
			return -1;
		}

		strcat (dest, current_header.name);

		dest[strlen (dest) + 1] = '\0';
		dest[strlen (dest)] = header_name_value_delimiter;

		strcat (dest, current_header.value);
		strcat (dest, line_delimiter);
	}

	strcat (dest, line_delimiter);
	if (NULL != request_size) {
		*request_size = strlen (dest);
	}

	return 0;
}

int
decode_response (const char *response, struct http_response *dest,
		 uint32_t *headers_length) {
	bool valid;
	uint32_t response_index;
	uint32_t idx;
	uint32_t line_lenght;
	uint32_t delimiter_index;

	if (NULL == response) {
		return -1;
	}

	if (NULL == dest) {
		return -1;
	}

	valid = false;
	for (response_index = 0; response[response_index] != '\0';
	     ++response_index) {
		if (response_index < 3) {
			continue;
		}

		if ((response[response_index - 3] == line_delimiter[0]) &&
		    (response[response_index - 2] == line_delimiter[1]) &&
		    (response[response_index - 1] == line_delimiter[0]) &&
		    (response[response_index] == line_delimiter[1])) {
			valid = true;
			break;
		}
	}
	if ((!valid) || (response_index < 3)) {
		return -1;
	} else {
		if (NULL != headers_length) {
			*headers_length = response_index;
		}
	}

	response_index = 0;
	while (is_whitespace (response[response_index])) {
		++response_index;
	}

	/* Check HTTP version string */
	idx = 0;
	while (http_version_text[idx] != '\0') {
		if (response[response_index] != http_version_text[idx]) {
			return -1;
		}
		++idx;
		++response_index;
	}
	dest->is_version11 = ('1' == response[response_index]);
	++response_index;

	while (is_whitespace (response[response_index])) {
		++response_index;
	}

	/* Check code */
	dest->code = ((uint16_t) response[response_index] - '0') * 100;
	++response_index;
	dest->code += (response[response_index] - '0') * 10;
	++response_index;
	dest->code += response[response_index] - '0';
	++response_index;

	response_index = next_line_start (response, response_index);

	/* Iterate through headers */
	if (0 != dest->header_count) {
		/* Check for valid pointers in the headers */
		for (idx = 0; idx < dest->header_count; ++idx) {
			if (NULL == dest->headers) {
				return -1;
			} else {
				if ((NULL == dest->headers[idx].name) ||
				    (NULL == dest->headers[idx].value)) {
					return -1;
				}
			}
		}

		line_lenght =
		  next_line_start (response,
				   response_index) - response_index - 2;
		while (line_lenght > 0) {
			/*
			 * For each header in dest
			 * STRNCMP name from start to ':' - 1
			 * if(true)
			 * STRNCPY from ':' + 1 to end
			 */
			for (idx = 0; idx < dest->header_count; ++idx) {
				delimiter_index =
				  next_delimiter (response, response_index);

				if (0 ==
				    strncmp (dest->headers[idx].name,
					     &(response[response_index]),
					     delimiter_index - response_index -
					     1)) {
					response_index = delimiter_index + 1;
					while (is_whitespace
					       (response[response_index])) {
						++response_index;
					}
					delimiter_index =
					  next_line_start (response,
							   response_index) - 2;
					strncpy (dest->headers[idx].value,
						 &(response[response_index]),
						 delimiter_index -
						 response_index);
					dest->headers[idx].
					  value[delimiter_index -
						response_index] = '\0';
					break;
				}
			}
			response_index =
			  next_line_start (response, response_index);
			line_lenght =
			  next_line_start (response,
					   response_index) - response_index - 2;
		}
	}
	return 0;
}

int
build_upload_payload_header (const char *filename, const char *form_name,
			     const char *boundary, char *header,
			     uint32_t *header_size) {
	/*
	 * Format of the header:
	 * --[boundary]\r\n
	 * Content-Disposition: form-data; name=\"[formName]\"; filename=\"[filename]\"\r\n
	 * Content-Type: application/octet-stream\r\n
	 * \r\n
	 */

	if (NULL == filename) {
		return -1;
	}

	if (NULL == form_name) {
		return -1;
	}

	if (NULL == boundary) {
		return -1;
	}

	if (NULL == header) {
		return -1;
	}

	strcpy (header, boundary_delimiter);
	strcat (header, boundary);
	strcat (header, line_delimiter);
	strcat (header, content_header_part1);
	strcat (header, form_name);
	strcat (header, content_header_part2);
	strcat (header, filename);
	strcat (header, "\"");
	strcat (header, line_delimiter);
	strcat (header, line_delimiter);
	strcat (header, content_header_part3);
	strcat (header, line_delimiter);

	if (NULL != header_size) {
		*header_size = strlen (header);
	}

	return 0;
}

int
build_upload_payload_trailer (const char *boundary, char *trailer,
			      uint32_t *trailer_size) {
	/*
	 * Format of the trailer:
	 * \r\n--[boundary]--\r\n
	 */

	if (NULL == boundary) {
		return -1;
	}

	if (NULL == trailer) {
		return -1;
	}

	strcpy (trailer, line_delimiter);
	strcat (trailer, boundary_delimiter);
	strcat (trailer, boundary);
	strcat (trailer, boundary_delimiter);
	strcat (trailer, line_delimiter);

	if (NULL != trailer_size) {
		*trailer_size = strlen (trailer);
	}

	return 0;
}
