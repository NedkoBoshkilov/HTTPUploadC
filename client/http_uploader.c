#include "comms.h"
#include "http_uploader.h"
#include "HTTPFuncs.h"
#include <stdint.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// STRLEN
#include <string.h>

#define BOUNDARY_DASH_COUNT 16
#define BOUNDARY_HEX_COUNT 16

#define BUFFER_SIZE 1024

static uint32_t currentNumber;
static uint32_t multiplier;
static uint32_t adder;

static uint32_t getContentHeaderSize(uint32_t filenameLength, uint32_t formNameLength)
{
	return 100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + filenameLength + formNameLength;
}

static uint32_t getContentTrailerSize()
{
	return 8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT;
}

static void generateBoundary(char* dest)
{
	int i, j, compare;
	const int fullRounds = BOUNDARY_HEX_COUNT / 8;
	const int remainder = BOUNDARY_HEX_COUNT % 8;
	uint32_t randomNumber;
	uint32_t mask = 0x0000000F;
	const char dash = '-';
	const char hex[] =
	{
		'0', '1', '2', '3',
		'4', '5', '6', '7',
		'8', '9', 'A', 'B',
		'C', 'D', 'E', 'F',
	};
	
	// ADD DASHES
	for(i = 0; i < BOUNDARY_DASH_COUNT; ++i)
	{
		dest[i] = dash;
	}
	
	// ADD RANDOM HEX SYMBOLS
	for(i = 0; i < fullRounds + 1; ++i)
	{
		if(i < fullRounds)
		{
			compare = 8;
		}
		else
		{
			compare = remainder;
		}
		randomNumber = generateNumber();

		for(j = 0; j < compare; ++j)
		{
			dest[i * 8 + j + BOUNDARY_DASH_COUNT] = hex[randomNumber & mask];
			randomNumber >>= 4;
		}
	}
	
	// NULL TERMINATION
	dest[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT] = '\0';	
}

static void getFilenameFromPath(char* filename, const char* filepath)
{
	const char delimiters[2] = {'/', '\\'};
	int start, end, i;
	start = 0;
	for(end = 0; filepath[end] != '\0'; ++end)
	{
		if( (filepath[end] == delimiters[0]) || (filepath[end] == delimiters[1]) )
		{
			start = end + 1;
		}
	}

	for(i = 0; i < end - start; ++i)
	{
		filename[i] = filepath[i + start];
	}
}

static int getFileSize(int fd)
{
	int result = 0;

	result = lseek(fd, 0, SEEK_END);

	if(result)
	{
		if( 0 != lseek(fd, 0, SEEK_SET) )
		{
			result = -1;
		}
	}

	return result;
}

static int buildFullHost(char* fullHost, const char* host, uint16_t port)
{
	int i = 0;
	int idx = 0;
	int divisor = 0;

	// Copy Host
	for(i = 0; host[i] != '\0'; ++i)
	{
		if(i > 14)
		{
			return -1;
		}

		fullHost[idx] = host[i];
		++idx;
	}

	fullHost[idx] = ':';
	++idx;

	// Put port in
	divisor = 10000;
	while( !(port/divisor) )
	{
		divisor /= 10;
	}

	while(divisor != 0)
	{
		fullHost[idx] = '0' + port / divisor;
		++idx;
		port = port % divisor;
		divisor /= 10;
	}
	fullHost[idx] = '\0';

	return 0;
}

static int generateAndSendRequest(int socket, const char* host, uint16_t port, const char* uri, uint32_t uriLength, uint8_t version, const char* boundary, uint32_t contentLenght)
{
	int result = 0;

	int idx = 0;
	uint32_t divisor = 0;
	uint32_t i = 0;

	uint8_t request[159 + 2 * uriLength + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	uint32_t requestSize = 0;
	HTTPRequest requestParams;
	HTTPHeader headers[4];

	char fullHost[22] = {0};
	char referer[8] = "Referer";
	char refererValue[21 + uriLength + 1];
	char type[13] = "Content-Type";
	char typeValue[30 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1] = "multipart/form-data; boundary=";
	char length[15] = "Content-Length";
	// 2^32 is 10 Digits
	char lengthValue[11] = {0};
	char conn[11] = "Connection";
	char connValue[11] = "keep-alive";

	// Create fullHost
	if( buildFullHost(fullHost, host, port) )
	{
		return -1;
	}

	// Fill refererValue
	idx = 0;
	for(i = 0; fullHost[i] != '\0'; ++i)
	{
		refererValue[idx] = fullHost[i];
		++idx;
	}

	for(i = 0; uri[i] != '\0'; ++i)
	{
		refererValue[idx] = uri[i];
		++idx;
	}
	refererValue[idx] = '\0';

	// Add Boundary to typeValue
	for(i = 0; i < BOUNDARY_DASH_COUNT+BOUNDARY_HEX_COUNT; ++i)
	{
		typeValue[30+i] = boundary[i];
	}

	// Fill lengthValue
	divisor = 1000000000;
	idx = 0;
	while( !(contentLenght / divisor) )
	{
		divisor /= 10;
	}

	while(divisor != 0)
	{
		lengthValue[idx] = '0' + contentLenght / divisor;
		++idx;
		contentLenght = contentLenght % divisor;
		divisor /= 10;
	}
	lengthValue[idx] = '\0';

	requestParams.method[0] = 'P';
	requestParams.method[1] = 'O';
	requestParams.method[2] = 'S';
	requestParams.method[3] = 'T';
	requestParams.method[4] = '\0';

	requestParams.isVersion11 = version;
	requestParams.uri = uri;
	requestParams.host = fullHost;
	requestParams.headerCount = 4;
	requestParams.headers = headers;

	headers[0].name = referer;
	headers[0].value = refererValue;
	headers[1].name = type;
	headers[1].value = typeValue;
	headers[2].name = length;
	headers[2].value = lengthValue;
	headers[3].name = conn;
	headers[3].value = connValue;

	result = buildRequest(&requestParams, request, &requestSize);
	if(0 == result)
	{
		result = writeData(socket, request, requestSize);
		if(requestSize == result)
		{
			result = 0;
		}
	}

	return result;
}

static int generateAndSendContentHeader(int socket, const char* filename, uint8_t filenameLength, const char* formName, uint8_t formNameLength, const char* boundary)
{
	int result = 0;

	const int headerSize = 100 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + filenameLength + formNameLength + 1;
	uint8_t header[headerSize];

	result = buildUploadPayloadHeader(filename, formName, boundary, header, NULL);

	if(0 == result)
	{
		result = writeData(socket, header, headerSize - 1);
		if(headerSize - 1 == result)
		{
			result = 0;
		}
	}

	return result;
}

static int generateAndSendContentTrailer(int socket, const char* boundary)
{
	int result = 0;

	const int trailerSize = 8 + BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1;
	uint8_t trailer[trailerSize];

	result = buildUploadPayloadTrailer(boundary, trailer, NULL);

	if(0 == result)
	{
		result = writeData(socket, trailer, trailerSize - 1);
		if(trailerSize - 1 == result)
		{
			result = 0;
		}
	}

	return result;
}

static int sendData(int fd, int socket)
{
	int result = 0;

	int bytesRead;
	int bytesWritten;
	char buffer[BUFFER_SIZE];

	bytesRead = read(fd, buffer, BUFFER_SIZE);

	while(bytesRead > 0)
	{
		bytesWritten = writeData(socket, buffer, bytesRead);

		if(bytesWritten != bytesRead)
		{
			result = -1;
			break;
		}

		bytesRead = read(fd, buffer, BUFFER_SIZE);
	}

	if(bytesRead < 0)
	{
		result = -1;
	}

	return result;
}

static int receiveAndDecode(int socket)
{
	int result = 0;

	uint8_t buff[18] = {0};
	uint8_t bytesRead = 0;
	char theVoid;

	HTTPResponse response;

	response.headerCount = 0;

	while(!bytesRead)
	{
		bytesRead = readData(socket, buff, 13);
	}

	if(bytesRead < 0)
	{
		return -1;
	}

	while(bytesRead != 13)
	{
		result = readData(socket, buff + bytesRead, 13 - bytesRead);
		if(result < 0)
		{
			return -1;
		}
		bytesRead += result;
	}

	result = 0;
	while(result = readData(socket, &theVoid, 1))
	{
		if(result < 0)
		{
			return -1;
		}
	}

	buff[13] = '\r';
	buff[14] = '\n';
	buff[15] = '\r';
	buff[16] = '\n';
	buff[17] = '\0';

	result = decodeResponse(buff, &response, NULL);

	if(result)
	{
		return result;
	}

	if( 2 != (response.code / 100) )
	{
		result = response.code;
	}

	return result;
}

int uploadFile(const char* filepath, const char* formName, const char* host, uint16_t port, const char* uri)
{
	int error = 0;

	int fileDescriptor;
	int fileSize;
	int socketDescriptor;
	char boundary[BOUNDARY_DASH_COUNT + BOUNDARY_HEX_COUNT + 1];
	char filename[NAME_MAX] = {0};
	uint32_t hostAddr;

	// PREPARE PART
	generateBoundary(boundary);
	getFilenameFromPath(filename, filepath);
	

	fileDescriptor = open(filepath, O_RDONLY);
	if(fileDescriptor < 0)
	{
		return -1;
	}

	fileSize = getFileSize(fileDescriptor);
	if(fileSize < 0)
	{
		close(fileDescriptor);
		return -1;
	}

	// SEND PART
	socketDescriptor = openSocket();
	if(socketDescriptor < 0)
	{
		close(fileDescriptor);
		return -1;
	}

	error = connectToAddr(socketDescriptor, host, port);
	if(error)
	{
		closeSocket(socketDescriptor);
		close(fileDescriptor);
		return -1;
	}

	error = generateAndSendRequest(socketDescriptor, host, port, uri, strlen(uri), 1, boundary,
		fileSize + getContentHeaderSize( strlen(filename), strlen(formName) ) + getContentTrailerSize());
	if(error)
	{
		closeSocket(socketDescriptor);
		close(fileDescriptor);
		return -1;
	}
	
	error = generateAndSendContentHeader(socketDescriptor, filename, strlen(filename), formName, strlen(formName), boundary);
	if(error)
	{
		closeSocket(socketDescriptor);
		close(fileDescriptor);
		return -1;
	}

	error = sendData(fileDescriptor, socketDescriptor);
	if(error)
	{
		closeSocket(socketDescriptor);
		return -1;
	}

	error = generateAndSendContentTrailer(socketDescriptor, boundary);
	if(error)
	{
		closeSocket(socketDescriptor);
		return -1;
	}

	// RECV PART
	error = receiveAndDecode(socketDescriptor);

	// CLEAN PART
	close(fileDescriptor);
	closeSocket(socketDescriptor);

	return error;
}

uint32_t generateNumber()
{
	currentNumber = (currentNumber * multiplier) + adder;
	return currentNumber;	
}

void setSeed(uint32_t startValue, uint32_t multiplicative, uint32_t additive)
{
	uint8_t fixer;

	currentNumber = startValue;
	fixer = (multiplicative - 1) % 4;
	if(fixer != 0)
	{
		multiplicative -= fixer;
	}
	multiplier = multiplicative;

	fixer = additive % 2;
	if(fixer != 1)
	{
		adder += 1;
	}
	adder = additive;
}