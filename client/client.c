#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// SOCKET
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "HTTPFuncs.h"

//Target host details:
#define PORT 8000
#define HOST "127.0.0.1"

char host[] = "127.0.0.1:8000";
//char uri[] = "http://127.0.0.1:8000/";
char uri[] = "/";

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

void prepareRequestStruct(HTTPRequest* request, HTTPHeader* header, char** buff, uint32_t fileSize, uint32_t contentHeaderSize, uint32_t contentTrailerSize)
{
	strcpy(buff[0], "Referer");
	strcpy(buff[1], "http://127.0.0.1:8000/");
	strcpy(buff[2], "Content-Type");
	strcpy(buff[3], "multipart/form-data; boundary=LOL");
	strcpy(buff[4], "Content-Length");
	sprintf(buff[5], "%u", fileSize + contentHeaderSize + contentTrailerSize ); // TODO: REMOVE?
	strcpy(buff[6], "Connection");
	strcpy(buff[7], "keep-alive");
	
	request->headerCount = 4;
	request->headers = header;
	header[0].name = buff[0];
	header[0].value = buff[1];
	header[1].name = buff[2];
	header[1].value = buff[3];
	header[2].name = buff[4];
	header[2].value = buff[5];
	header[3].name = buff[6];
	header[3].value = buff[7];
		
	request->host = host;
	request->uri = uri;
	request->isVersion11 = 1;
	strcpy(request->method, "POST");
}

void prepareResponseStruct(HTTPResponse* response, HTTPHeader* responseHeader, char** responseBuffs)
{
    strcpy(responseBuffs[0], "Content-Length");
    
    responseHeader->name = responseBuffs[0];
    responseHeader->value = responseBuffs[1];
    
    response->headerCount = 1;
    response->headers = responseHeader;
}

int main(int argc, char** argv)
{	
	if(argc < 2)
	{
		printf("Usage: upload [filename]\n");
		return 0;
	}
	
	// GET FILE SIZE
	uint32_t fileSize = 0;
	
	FILE* file = fopen(argv[1], "r");
	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	// PREPARE CONTENT ENCAPSULATION
	const char boundary[] = "LOL";
	uint8_t contentHeader[1024] = {0};
	uint8_t contentTrailer[1024] = {0};
	uint32_t contentHeaderSize = 0;
	uint32_t contentTrailerSize = 0;
	buildUploadPayloadEncapsulation(argv[1], boundary, contentHeader, contentTrailer, &contentHeaderSize, &contentTrailerSize);

	// PREPARE REQUEST
	HTTPRequest request;
	HTTPHeader header[4];
	char buffer0[256] = {0};
	char buffer1[256] = {0};
	char buffer2[256] = {0};
	char buffer3[256] = {0};
	char buffer4[256] = {0};
	char buffer5[256] = {0};
	char buffer6[256] = {0};
	char buffer7[256] = {0};
	
	char* buff[8] =
	{
		buffer0, buffer1, buffer2, buffer3,
		buffer4, buffer5, buffer6, buffer7
	};
	prepareRequestStruct(&request, header, buff, fileSize, contentHeaderSize, contentTrailerSize);
	
	// PREPARE RESPONSE
	char responseBuffer0[256] = {0};
	char responseBuffer1[256] = {0};
	
	char* responseBuffs[2] = {responseBuffer0, responseBuffer1};
    HTTPResponse response;
    HTTPHeader responseHeader;
    prepareResponseStruct(&response, &responseHeader, responseBuffs);
    
	// CREATE SOCKET
	int sd = createSocket();
    
    // SEND
    uint8_t reqStr[1024] = {0};
    uint32_t reqSize;
    buildRequest(&request, reqStr, &reqSize);
    
    write(sd, (void*)reqStr, reqSize);
    write(sd, contentHeader, contentHeaderSize);
    
    uint8_t fileBUFF[fileSize];
    fread(&fileBUFF, 1, fileSize, file);
    
    write(sd, fileBUFF, fileSize);
    write(sd, contentTrailer, contentTrailerSize);
    
    printf("Press Enter to view response");
    getchar();
    
    // RECV
    char respBuff[2048] = {0};
    read(sd, respBuff, 2048);
    
    decodeResponse( (const uint8_t*) respBuff, &response, NULL);
    
    printf("--- RESPONSE ---\n");
    printf("Code: %d\n", response.code);
    printf("%s: %s\n", response.headers->name, response.headers->value);
    
    close(sd);
    return 0;
}