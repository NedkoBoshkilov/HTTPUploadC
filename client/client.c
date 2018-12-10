#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "HTTPFuncs.h"

// SOCKET
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#define FUNCTION_NAME(name) #name

//Target host details:
#define HOST "127.0.0.1"
#define PORT 8000
#define PORTSTR FUNCTION_NAME(PORT)
char uri[] = "/";

#define SMALL_BUFFER_SIZE 256
#define BIG_BUFFER_SIZE 1024

int createSocket()
{
	// SOCKET PREPARATIONS
	int sd, ret;
    struct sockaddr_in server;
    struct in_addr ipv4addr;
    struct hostent *hp;

    sd = socket(AF_INET,SOCK_STREAM,0);
    server.sin_family = AF_INET;

    inet_pton(AF_INET, HOST, &ipv4addr);
    hp = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);

    bcopy(hp->h_addr, &(server.sin_addr.s_addr), hp->h_length);
    server.sin_port = htons(PORT);

	connect(sd, (const struct sockaddr *)&server, sizeof(server));
	return sd;
}

int sendFile(int socket, const char* filename)
{	
	FILE* file;
	uint32_t fileSize = 0;
	uint32_t remaining = fileSize;
	uint8_t fileBUFF[BIG_BUFFER_SIZE];
	
	char boundary[11]; // TODO: ALGORITHM FOR BOUNDARY GENERATION
	uint8_t contentHeader[BIG_BUFFER_SIZE];
	uint8_t contentTrailer[BIG_BUFFER_SIZE];
	uint32_t contentHeaderSize = 0;
	uint32_t contentTrailerSize = 0;
	
	HTTPRequest request;
	HTTPHeader header[4];
	char buff[8][SMALL_BUFFER_SIZE];
	char host[SMALL_BUFFER_SIZE];
	
	uint8_t requestStr[BIG_BUFFER_SIZE];
    uint32_t requestSize = 0;;
    
    uint32_t written = 0;
    uint32_t bytesRead = 0;
    
    HTTPResponse response;
    char responseStr[BIG_BUFFER_SIZE];
    uint16_t responseIdx = 0;
    int errCode = 0;
	
	// OPEN FILE
	file = fopen(filename, "r");
	
	if(!file)
	{
		return -1; // ERROR OPENING FILE
	}
		
	// GET FILE SIZE
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	remaining = fileSize;

	strcpy(boundary, "LOL");

	// PREPARE CONTENT ENCAPSULATION
	errCode = buildUploadPayloadEncapsulation(filename, boundary, contentHeader, contentTrailer, &contentHeaderSize, &contentTrailerSize);
	if( errCode )
	{
		return -2; // ERROR BUILDING ENCAPSULATION
	}
	
	// BUILD HOST STRING
	strcpy(host, HOST);
	strcat(host, ":");
	strcat(host, PORTSTR);
	
	// BUILD REQUEST HEADERS
	strcpy(buff[0], "Referer");
	strcpy(buff[1], host);
	strcat(buff[1], uri);
	strcpy(buff[2], "Content-Type");
	strcpy(buff[3], "multipart/form-data; boundary=");
	strcat(buff[3], boundary);
	strcpy(buff[4], "Content-Length");
	sprintf(buff[5], "%u", fileSize + contentHeaderSize + contentTrailerSize ); // TODO: REMOVE SPRINTF?
	strcpy(buff[6], "Connection");
	strcpy(buff[7], "keep-alive");
	
	request.headerCount = 4;
	request.headers = header;
	header[0].name = buff[0];
	header[0].value = buff[1];
	header[1].name = buff[2];
	header[1].value = buff[3];
	header[2].name = buff[4];
	header[2].value = buff[5];
	header[3].name = buff[6];
	header[3].value = buff[7];
	
	request.host = host;
	request.uri = uri;
	request.isVersion11 = 1;
	strcpy(request.method, "POST");
	
	// BUILD REQUEST
	errCode = buildRequest(&request, requestStr, &requestSize);
    if( errCode )
    {
		return -3; // ERROR BUILDING REQUEST
	}
    
    // SEND
    written += write(socket, requestStr, requestSize);
    written += write(socket, contentHeader, contentHeaderSize);
    
    while(remaining > BIG_BUFFER_SIZE)
    {
		// GET FILE CONTENT
		fread(&fileBUFF, 1, BIG_BUFFER_SIZE, file);
		written += write(socket, fileBUFF, BIG_BUFFER_SIZE);
		remaining -= BIG_BUFFER_SIZE;
	}
	
	fread(&fileBUFF, 1, remaining, file);
	written += write(socket, fileBUFF, remaining);
    
    written += write(socket, contentTrailer, contentTrailerSize);
    if(written < requestSize + contentHeaderSize + fileSize + contentTrailerSize)
    {
		return -4; // ERROR SENDING DATA
	}
	
	// CLOSE FILE
    fclose(file);
    
    // PREPARE RESPONSE
    response.headerCount = 0;
     
    // RECEIVE
    
    // WAIT FOR DATA
    do
    {
		bytesRead = read(socket, responseStr, 1);
	}
    while(0 == bytesRead);
    
    ++responseIdx;
    
    // READ ALL DATA
    while(bytesRead != 0)
    {
		bytesRead = read(socket, responseStr + responseIdx, BIG_BUFFER_SIZE - responseIdx);
		responseIdx += bytesRead;
	}	
    
    errCode = decodeResponse( (const uint8_t*) responseStr, &response, NULL);
    if( errCode )
    {
		return -5; // ERROR DECODING RESPONSE
	}
    
    if(response.code / 100 == 2)
    {
		return 0; // OK
	}
	else
	{
		return response.code; // SENDING WAS SUCCESSFUL BUT OK WAS NOT RECEIVED
	}
}

int main(int argc, char** argv)
{	
	int socket;
	int code;
	
	if(argc < 2)
	{
		printf("Usage: upload [filename]\n");
		return 0;
	}
	
	// CREATE SOCKET
	socket = createSocket();
	
	// SEND FILE
	code = sendFile(socket, argv[1]);
	if( 0 == code )
	{
		printf("OK\n");
	}
	else
	{
		printf("Error sending file %s: %d\n", argv[1], code);
	}
	
	// CLOSE SOCKET
	close(socket);
    
    return 0;
}