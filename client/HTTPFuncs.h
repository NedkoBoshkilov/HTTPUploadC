#ifndef HTTP_FUNCTIONS_H_
#define HTTP_FUNCTIONS_H_

#include "HTTPDefines.h"

// Params - Filled HTTPRequest structure
// dest - the request string will be output
// requestSize - length of the request string - can be NULL
int8_t buildRequest(const HTTPRequest* params, uint8_t* dest, uint32_t* requestSize);

// response - response string to decode
// dest - HTTPResponse structure to be filled
int8_t decodeResponse(const uint8_t* response, HTTPResponse* dest, uint32_t* headersLength);

#endif //HTTP_FUNCTIONS_H_