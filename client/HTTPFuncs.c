#include "HTTPFuncs.h"

#include <string.h>

const uint8_t delimiter[2] = {0x0D, 0x0A};
const char headerNameValueDelimiter = ':';
const char HTTPVersionSTR[] = "HTTP/1.";
const char hostHeaderName[] = "Host";

uint8_t isWhitespace(char test)
{
	uint8_t result = 0;
	const char whitespace[] = {' ', '\t', '\v', '\f'};
	for(uint8_t i = 0; i < 4; ++i)
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
		if( (buff[result] == delimiter[1]) &&
			(buff[result - 1] == delimiter[0]) )
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
	uint32_t destIndex = 0;
	uint32_t i = 0;
	
	if(!params)
	{
		return -1; //BAD PARAMS
	}
	
	if(!dest)
	{
		return -1; //BAD DEST
	}
	
	// SET METHOD
	i = 0;
	while(params->method[i] != '\0')
	{
		// CAST
		dest[destIndex] = (uint8_t) (params->method[i]);
		++i;
		++destIndex;
	}
	
	// ADD SPACE
	dest[destIndex] = ' ';
	++destIndex;
	
	// SET URI
	i = 0;
	while(params->uri[i] != '\0')
	{
		// CAST
		dest[destIndex] = (uint8_t) (params->uri[i]);
		++i;
		++destIndex;
	}
	
	// ADD SPACE
	dest[destIndex] = ' ';
	++destIndex;
	
	// SET VERSION
	i = 0;
	while(HTTPVersionSTR[i] != '\0')
	{
		// CAST
		dest[destIndex] = (uint8_t) (HTTPVersionSTR[i]);
		++i;
		++destIndex;
	}
	if(params->isVersion11)
	{
		dest[destIndex] = '1';
	}
	else
	{
		dest[destIndex] = '0';
	}
	++destIndex;
	
	// NEW LINE
	dest[destIndex] = delimiter[0];
	++destIndex;
	dest[destIndex] = delimiter[1];
	++destIndex;
	
	// SET HOST HEADER
	i = 0;
	while(hostHeaderName[i] != '\0')
	{
		// CAST
		dest[destIndex] = (uint8_t) (hostHeaderName[i]);
		++i;
		++destIndex;
	}
	
	// CAST
	dest[destIndex] = (uint8_t) headerNameValueDelimiter;
	++destIndex;
	
	i = 0;
	while(params->host[i] != '\0')
	{
		// CAST
		dest[destIndex] = (uint8_t) (params->host[i]);
		++i;
		++destIndex;
	}
	
	// NEW LINE
	dest[destIndex] = delimiter[0];
	++destIndex;
	dest[destIndex] = delimiter[1];
	++destIndex;
	
	// SET OTHER HEADERS
	for(uint8_t idxHeader = 0; idxHeader < params->headerCount; ++idxHeader)
	{
		if(!(params->headers))
		{
			return -1; //BAD HEADERS
		}
		
		HTTPHeader curHeader = params->headers[idxHeader];
		
		// NAME
		i = 0;
		while(curHeader.name[i] != '\0')
		{
			// CAST
			dest[destIndex] = (uint8_t) (curHeader.name[i]);
			++i;
			++destIndex;
		}
		
		// CAST
		dest[destIndex] = (uint8_t) headerNameValueDelimiter;
		++destIndex;
		
		// VALUE
		i = 0;
		while(curHeader.value[i] != '\0')
		{
			// CAST
			dest[destIndex] = (uint8_t) (curHeader.value[i]);
			++i;
			++destIndex;
		}
		
		// NEW LINE
		dest[destIndex] = delimiter[0];
		++destIndex;
		dest[destIndex] = delimiter[1];
		++destIndex;
	}

	// NEW LINE
	dest[destIndex] = delimiter[0];
	++destIndex;
	dest[destIndex] = delimiter[1];
	++destIndex;
	
	// SET PAYLOAD
	/*if(params->payloadSize != 0)
	{
		if(!(params->payload))
		{
			return -1; //BAD PAYLOAD
		}
		i = 0;
		while(i < params->payloadSize)
		{
			dest[destIndex] = params->payload[i];
			++i;
			++destIndex;
		}
	}*/
	
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
	uint32_t i;
	
	if(!response)
	{
		return -1; // BAD RESPONSE
	}
	
	if(!dest)
	{
		return -2; // BAD DEST
	}
	
	// Valid Response?
	uint8_t valid = 0;
	for(i = 0; response[i] != '\0'; ++i)
	{
		if(i < 3)
		{
			continue;
		}		
		
		if( (response[i - 3] == delimiter[0]) &&
			(response[i - 2] == delimiter[1]) &&
			(response[i - 1] == delimiter[0]) &&
			(response[i] == delimiter[1]) )
		{
			valid = 1;
			break;
		}
	}
	if( (!valid) || (i < 3) )
	{
		return -3; // BAD RESPONSE
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
			return -4; // BAD RESPONSE
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
		uint32_t lineLenght = nextLineStart(response, responseIndex) - responseIndex - 2;
		while(lineLenght)
		{
			char name[256] = {0};
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
				// TODO: Remove STRCMP
				if(0 == strcmp(dest->headers[i].name, name) )
				{
					do
					{
						++responseIndex;
					}
					while(isWhitespace(response[responseIndex]));
					//Copy value
					for(uint32_t idx = 0; response[responseIndex] != delimiter[0]; ++responseIndex)
					{
						dest->headers[i].value[idx] = response[responseIndex];
						++idx;
					}
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