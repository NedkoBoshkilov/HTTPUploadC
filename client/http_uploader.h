#ifndef HTTP_UPLOADER_H_
#define HTTP_UPLOADER_H_

#include <stdint.h>

int uploadFile(const char* filepath, const char* formName, const char* host, uint16_t port, const char* uri);

uint32_t generateNumber();

void setSeed(uint32_t startValue, uint32_t multiplicative, uint32_t additive);

#endif // HTTP_UPLOADER_H_