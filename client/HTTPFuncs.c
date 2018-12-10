#include "HTTPFuncs.h"

// TODO: REMOVE
#include <string.h>

const char lineDelimiter[] = "\r\n";
const char HTTPVersionSTR[] = "HTTP/1.";
const char hostHeaderName[] = "Host";
const uint8_t headerNameValueDelimiter = ':';

const char boundaryDelimiter[] = "--";
const char contentHeader1[] = "Content-Disposition: form-data; name=\"file\"; filename=\"";
const char contentHeader2[] = "Content-Type: application/octet-stream";

void copyString(uint8_t* dest, const char* src, uint32_t* destIdx)
{
	uint32_t i;
	for(i = 0; src[i] != '\0'; ++i)
	{
		dest[*destIdx] = src[i];
		++(*destIdx);
	}
}

uint8_t isWhitespace(char test)
{
	uint8_t result = 0;
	uint32_t i;
	const char whitespace[] = {' ', '\t', '\v', '\f'};
	for(i = 0; i < 4; ++i)
	{
		if(whitespace[i] == test)
		{
			result = 1;
		}
	}
	return result;
}

uint32_t nextLineStart(const uint8_t* buff, uint32_t start)
{
	uint32_t result = start;
	uint8_t found = 0;
	while(!found)
	{
		if( (buff[result] == lineDelimiter[1]) &&
			(buff[result - 1] == lineDelimiter[0]) )
		{
			found = 1;
		}
		++result;
	}
	return result;
}

uint32_t nextDelimiter(const uint8_t* buff, uint32_t start)
{
	uint32_t result = 0;
	uint8_t found = 0;
	while(!found)
	{
		if(buff[result] == headerNameValueDelimiter)
		{
			found = 1;
		}
		++result;
	}
	return result;
}

int8_t buildRequest(const HTTPRequest* params, uint8_t* dest, uint32_t* requestSize)
{
	// [method] [uri] HTTP/1.X\r\n
	// Host:[host]\r\n
	// [Name]:[value]\r\n
	// \r\n
	
	uint32_t destIndex = 0;
	uint32_t i = 0;
	uint8_t idxHeader = 0;
	
	if(!params)
	{
		return -1; //BAD PARAMS
	}
	
	if(!dest)
	{
		return -2; //BAD DEST
	}
	
	// SET FIRST LINE
	copyString(dest, params->method, &destIndex);
	
	dest[destIndex] = ' ';
	++destIndex;
	
	copyString(dest, params->uri, &destIndex);
	
	dest[destIndex] = ' ';
	++destIndex;
	
	copyString(dest, HTTPVersionSTR, &destIndex);
	
	if(params->isVersion11)
	{
		dest[destIndex] = '1';
	}
	else
	{
		dest[destIndex] = '0';
	}
	++destIndex;
		
	copyString(dest, lineDelimiter, &destIndex);
	
	// SET HOST HEADER
	copyString(dest, hostHeaderName, &destIndex);
	
	dest[destIndex] = headerNameValueDelimiter;
	++destIndex;

	copyString(dest, params->host, &destIndex);
	copyString(dest, lineDelimiter, &destIndex);
	
	// SET OTHER HEADERS
	for(idxHeader = 0; idxHeader < params->headerCount; ++idxHeader)
	{
		if(!(params->headers))
		{
			return -1; //BAD PARAMS
		}
		
		HTTPHeader curHeader = (params->headers)[idxHeader];
		copyString(dest, curHeader.name, &destIndex);
		
		dest[destIndex] = headerNameValueDelimiter;
		++destIndex;
		
		copyString(dest, curHeader.value, &destIndex);
		copyString(dest, lineDelimiter, &destIndex);
	}

	copyString(dest, lineDelimiter, &destIndex);
	
	dest[destIndex] = '\0';
	if(requestSize)
	{
		*requestSize = destIndex;
	}
	
	return 0;
}

int8_t decodeResponse(const uint8_t* response, HTTPResponse* dest, uint32_t* headersLength)
{
	uint32_t responseIndex = 0;
	uint32_t i = 0;
	uint8_t valid = 0;
	uint32_t lineLenght = 0;
	char name[256] = {0};
	uint32_t idx = 0;
	
	if(!response)
	{
		return -1; // BAD RESPONSE
	}
	
	if(!dest)
	{
		return -2; // BAD DEST
	}
	
	// Valid Response?
	for(i = 0; response[i] != '\0'; ++i)
	{
		if(i < 3)
		{
			continue;
		}		
		
		if( (response[i - 3] == lineDelimiter[0]) &&
			(response[i - 2] == lineDelimiter[1]) &&
			(response[i - 1] == lineDelimiter[0]) &&
			(response[i] == lineDelimiter[1]) )
		{
			valid = 1;
			break;
		}
	}
	if( (!valid) || (i < 3) )
	{
		return -1; // BAD RESPONSE
	}
	else
	{
		if(headersLength)
		{
			*headersLength = i;
		}
	}
	
	// Remove whitespace
	while( isWhitespace(response[responseIndex]) )
	{
		++responseIndex;
	}
	
	// CHECK HTTP Version String
	i = 0;
	while(HTTPVersionSTR[i] != '\0')
	{
		if(response[responseIndex] != HTTPVersionSTR[i])
		{
			return -1; // BAD RESPONSE
		}
		++i;
		++responseIndex;
	}
	dest->isVersion11 = response[responseIndex] - '0';
	++responseIndex;
	
	// Remove whitespace
	while( isWhitespace(response[responseIndex]) )
	{
		++responseIndex;
	}
	
	// CHECK Code
	dest->code = ((uint16_t)response[responseIndex] - '0') * 100;
	++responseIndex;
	dest->code += (response[responseIndex] - '0') * 10;
	++responseIndex;
	dest->code += response[responseIndex] - '0';
	++responseIndex;
	
	// FIND NEXT LINE
	responseIndex = nextLineStart(response, responseIndex);
	
	// Iterate Headers
	if(dest->headerCount)
	{
		// FIND LINE LENGTH
		lineLenght = nextLineStart(response, responseIndex) - responseIndex - 2;
		while(lineLenght)
		{
			
			// HELLP
			memset(name, 0, 256);
			i = 0;
			// Find the name of the header
			while(response[responseIndex] != headerNameValueDelimiter)
			{
				name[i] = (char)response[responseIndex];
				++i;
				++responseIndex;
			}
			
			// Compare the names that we are looking for
			for(i = 0; i < dest->headerCount; ++i)
			{
				// TODO: REMOVE STRCMP
				if(0 == strcmp(dest->headers[i].name, name) )
				{
					do
					{
						++responseIndex;
					}
					while(isWhitespace(response[responseIndex]));
					
					//Copy value
					idx = 0;
					
					while(response[responseIndex] != lineDelimiter[0])
					{
						dest->headers[i].value[idx] = response[responseIndex];
						++idx;
						++responseIndex;
					}
					
					dest->headers[i].value[idx] = '\0';
					break;
				}
			}
			responseIndex = nextLineStart(response, responseIndex);
			//FIND LINE LENGTH
			lineLenght = nextLineStart(response, responseIndex) - responseIndex - 2;
		}
	}
	
	return 0;
}

int8_t buildUploadPayloadEncapsulation(const char* filename, const char* boundary, uint8_t* header, uint8_t* trailer, uint32_t* headerSize, uint32_t* trailerSize)
{
	// --[boundary]\r\n
	// Content-Disposition: form-data; name=\"file\"; filename=\"[filename]\"\r\n
	// Content-Type: application/octet-stream\r\n
	// \r\n
	// [data]
	// \r\n--[boundary]--\r\n
	
	uint16_t i;
	uint32_t headerIndex = 0;
	uint32_t trailerIndex = 0;
	
	if(!filename)
	{
		return -1; //BAD FILENAME
	}
	
	if( (!header) || (!trailer) )
	{
		return -2; //BAD DEST
	}
	
	// HEADER
	copyString(header, boundaryDelimiter, &headerIndex);
	
	// TRAILER
	copyString(trailer, lineDelimiter, &trailerIndex);
	copyString(trailer, boundaryDelimiter, &trailerIndex);
	
	
	// COPY STRING INTO BOTH HEADER AND TRAILER
	for(i = 0; boundary[i] != '\0'; ++i)
	{
		header[headerIndex] = boundary[i];
		++headerIndex;
		trailer[trailerIndex] = boundary[i];
		++trailerIndex;
	}
	
	// TRAILER
	copyString(trailer, boundaryDelimiter, &trailerIndex);
	copyString(trailer, lineDelimiter, &trailerIndex);
	
	trailer[trailerIndex] = '\0';
	if(trailerSize)
	{
		*trailerSize = trailerIndex;
	}
	// TRAILER END
	
	// HEADER
	copyString(header, lineDelimiter, &headerIndex);
	copyString(header, contentHeader1, &headerIndex);
	copyString(header, filename, &headerIndex);
	
	header[headerIndex] = '"';
	++headerIndex;
	
	copyString(header, lineDelimiter, &headerIndex);
	copyString(header, contentHeader2, &headerIndex);
	copyString(header, lineDelimiter, &headerIndex);
	copyString(header, lineDelimiter, &headerIndex);
	
	header[headerIndex] = '\0';
	if(headerSize)
	{
		*headerSize = headerIndex;
	}
	// HEADER END
	
	return 0;
}