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

const char HTTPMethods[9][8] = 
{
	"GET",
	"HEAD",
	"POST",
	"PUT",
	"DELETE",
	"CONNECT",
	"OPTIONS",
	"TRACE",
	"PATCH"
};

typedef enum HTTPMethodIndexes
{
	HTTP_REQUEST_GET = 0,
	HTTP_REQUEST_HEAD,
	HTTP_REQUEST_POST,
	HTTP_REQUEST_PUT,
	HTTP_REQUEST_DELETE,
	HTTP_REQUEST_CONNECT,
	HTTP_REQUEST_OPTIONS,
	HTTP_REQUEST_TRACE,
	HTTP_REQUEST_PATCH
} HTTPMethodIndexes;

#endif // HTTP_DEFINES_H_