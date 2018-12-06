#ifndef HTTP_DEFINES_H_
#define HTTP_DEFINES_H_

#include <stdint.h>

typedef struct HTTPHeader
{
	char* name;
	char* value;
} HTTPHeader;

typedef struct HTTPRequest
{
	char method[8];
	uint8_t isVersion11;
	char* uri;
	char* host;
	uint8_t headerCount;
	HTTPHeader* headers;
} HTTPRequest;

typedef struct HTTPResponse
{
	uint8_t isVersion11;
	uint16_t code;
	uint8_t headerCount;
	HTTPHeader* headers;
} HTTPResponse;

#endif // HTTP_DEFINES_H_