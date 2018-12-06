#include <iostream>
#include <fstream>
#include "HTTPFuncs.h"
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cstring>

// SOCKET
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

//Target host details:
#define PORT 8000
#define HOST "127.0.0.1"

using namespace std;

int main(int argc, char** argv)
{	
	if(argc < 2)
	{
		cout << "Usage: upload [filename]" << endl;
		return 0;
	}
	
	string filename = argv[1];
	ifstream file(filename.c_str(), ios_base::in | ios_base::binary);
	
	file.seekg(0, file.end);
	int size = file.tellg();
	file.seekg(0, file.beg);

	char boundary[] = "LOL";
	
	uint8_t contentHeader[1024] = {0};
	uint8_t contentTrailer[1024] = {0};
	uint32_t contentHeaderSize = 0;
	uint32_t contentTrailerSize = 0;
	buildUploadPayloadEncapsulation(argv[1], boundary, contentHeader, contentTrailer, &contentHeaderSize, &contentTrailerSize);

	HTTPRequest request;
	HTTPHeader header[4];
	char buff[8][256] = {0};
	strcpy(buff[0], "Referer");
	strcpy(buff[1], "http://127.0.0.1:8000/");
	strcpy(buff[2], "Content-Type");
	strcpy(buff[3], "multipart/form-data; boundary=LOL");
	strcpy(buff[4], "Content-Length");
	sprintf(buff[5], "%u", size + contentHeaderSize + contentTrailerSize ); // TODO: REMOVE?
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
		
	char host[] = "127.0.0.1:8000";
	//char uri[] = "http://127.0.0.1:8000/";
	char uri[] = "/";
	request.host = host;
	request.uri = uri;
	request.isVersion11 = 1;
	strcpy(request.method, HTTPMethods[HTTP_REQUEST_POST]);
	
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

	connect(sd, (const sockaddr *)&server, sizeof(server));
    
    // SEND
    uint8_t reqStr[1024];
    uint32_t reqSize;
    buildRequest(&request, reqStr, &reqSize);
    
    uint32_t written = write(sd, (void*)reqStr, reqSize);
    //cout << "HTTP Header: Written " << written << " bytes out of " << reqSize << endl;
    
    written = write(sd, contentHeader, contentHeaderSize);
    //cout << "Content Header: Written " << written << " bytes out of " << contentHeaderSize << endl;
    
    uint8_t fileBUFF[size];
    file.read((char*) fileBUFF, size);
    
    written = write(sd, fileBUFF, size);
    //cout << "Data: Written " << written << " bytes out of " << size << endl;
    
    written = write(sd, contentTrailer, contentTrailerSize);
    //cout << "Content Trailer: Written " << written << " bytes out of " << contentTrailerSize << endl;
    
    cout << "Press Enter to view response";
    getchar();
    
    // RECV
    char respBuff[2048] = {0};
    read(sd, respBuff, 2048);
    //cout << "*** RESPONSE ***" << endl << respBuff << endl;
    
    char responseBuffs[2][256] = {0};
    HTTPHeader responseHeaders;
    HTTPResponse response;
    
    strcpy(responseBuffs[0], "Content-Length");
    
    responseHeaders.name = responseBuffs[0];
    responseHeaders.value = responseBuffs[1];
    
    response.headerCount = 1;
    response.headers = &responseHeaders;
    
    decodeResponse( (const uint8_t*) respBuff, &response, NULL);
    
    cout << "--- RESPONSE ---" << endl;
    cout << "Code: " << response.code << endl;
    cout << responseHeaders.name << ": " << responseHeaders.value << endl;
    
    /*
    for(uint8_t i = 0; i < 0; ++i)
    {
		cout << responseHeaders[i].name << " - " << responseHeaders[i].value << endl;
	}
	*/
    
    close(sd);
    return 0;
}