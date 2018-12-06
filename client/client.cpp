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
	char newLine[] = "\r\n";
	char boundaryDelimiter[] = "--";
	
	char contentHeader[1024] = {0};
	strcpy(contentHeader, boundaryDelimiter);
	strcat(contentHeader, boundary);
	strcat(contentHeader, newLine);
	strcat(contentHeader, "Content-Disposition: form-data; name=\"file\"; filename=\"");
	strcat(contentHeader, filename.c_str());
	strcat(contentHeader, "\"");
	strcat(contentHeader, newLine);
	strcat(contentHeader, "Content-Type: image/png");
	strcat(contentHeader, newLine);
	strcat(contentHeader, newLine);
	
	cout << "Content Header: " << contentHeader << endl;
	
	char contentTrailer[1024] = {0};
	strcpy(contentTrailer, newLine);
	strcat(contentTrailer, boundaryDelimiter);
	strcat(contentTrailer, boundary);
	strcat(contentTrailer, boundaryDelimiter);
	strcat(contentTrailer, newLine);
	
	cout << "Content Trailer: " << contentTrailer << endl;

	HTTPRequest request;
	HTTPHeader header[5];
	char buff[10][256] = {0,0};
	strcpy(buff[0], "Referer");
	strcpy(buff[1], "http://127.0.0.1:8000/");
	strcpy(buff[2], "Content-Type");
	strcpy(buff[3], "multipart/form-data; boundary=LOL");
	strcpy(buff[4], "Content-Length");
	sprintf(buff[5], "%lu", size + strlen(contentHeader) + strlen(contentTrailer) ); //TODO: Remove
	strcpy(buff[6], "Connection");
	strcpy(buff[7], "keep-alive");
	strcpy(buff[8], "User-Agent");
	strcpy(buff[9], "fileUploader");
	
	
	request.headerCount = 5;
	request.headers = header;
	header[0].name = buff[0];
	header[0].value = buff[1];
	header[1].name = buff[2];
	header[1].value = buff[3];
	header[2].name = buff[4];
	header[2].value = buff[5];
	header[3].name = buff[6];
	header[3].value = buff[7];
	header[4].name = buff[8];
	header[4].value = buff[9];
		
	char host[] = "127.0.0.1:8000";
	//char uri[] = "http://127.0.0.1:8000/";
	char uri[] = "/";
	request.host = host;
	request.uri = uri;
	request.isVersion11 = 1;
	strcpy(request.method, HTTPMethods[HTTP_REQUEST_POST]);
	
	// SOCKET STUFF
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
    cout << "HTTP Header: Written " << write(sd, (void*)reqStr, reqSize) << " bytes out of " << reqSize << endl;
    
    cout << "Content Header: Written " << write(sd, contentHeader, strlen(contentHeader)) << " bytes out of " << strlen(contentHeader) << endl;
    
    uint8_t fileBUFF[size];
    file.read((char*) fileBUFF, size);
    cout << "Data: Written " << write(sd, fileBUFF, size) << " bytes out of " << size << endl;
    
    cout << "Content Trailer: Written " << write(sd, contentTrailer, strlen(contentTrailer)) << " bytes out of " << strlen(contentTrailer) << endl;
    
    getchar();
    
    // RECV
    char respBuff[2048] = {0};
    read(sd, respBuff, 2048);
    cout << "*** RESPONSE ***" << endl << respBuff << endl;
    
    return 0;
}