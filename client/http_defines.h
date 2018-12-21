#ifndef HTTP_DEFINES_H_
#define HTTP_DEFINES_H_

#include <stdbool.h>
#include <stdint.h>

struct http_header {
	char *name;
	char *value;
};

struct http_request {
	char method[8];
	const char *uri;
	const char *host;
	bool is_version11;
	uint8_t header_count;
	struct http_header *headers;
};

struct http_response {
	uint16_t code;
	uint8_t is_version11;
	uint8_t header_count;
	struct http_header *headers;
};

#endif /* HTTP_DEFINES_H_ */
