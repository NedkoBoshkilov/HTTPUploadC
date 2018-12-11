#ifndef HTTP_FUNCTIONS_H_
#define HTTP_FUNCTIONS_H_

#include "HTTPDefines.h"


// BUILDS AN HTTP REQUEST BASED ON PARAMETERS
// params - Filled HTTPRequest structure
// dest - the request string will be output
// requestSize - length of the request string - can be NULL
int8_t buildRequest(const HTTPRequest* params, uint8_t* dest, uint32_t* requestSize);

// DECODES AN HTTP RESPONSE AND GETS VALUES OF HEADERS SET IN PARAMETERS
// response - response string to decode
// dest - HTTPResponse structure to be filled
int8_t decodeResponse(const uint8_t* response, HTTPResponse* dest, uint32_t* headersLength);

// CREATES CONTENT DATA HEADER AND TRAILER FOR SINGLE FILE UPLOAD
// filename - name of the file
// boundary - content delimiting string
// header - content header string will be output
// trailer - content trailer string will be output
// dest1Size - length of the header string - can be NULL
// dest1Size - length of the trailer string - can be NULL
int8_t buildUploadPayloadEncapsulation(const char* filename, const char* formName, const char* boundary, uint8_t* header, uint8_t* trailer, uint32_t* headerSize, uint32_t* trailerSize);

int8_t buildUploadPayloadHeader(const char* filename, const char* formName, const char* boundary, uint8_t* header, uint32_t* headerSize);

int8_t buildUploadPayloadTrailer(const char* boundary, uint8_t* trailer, uint32_t* trailerSize);

#endif //HTTP_FUNCTIONS_H_