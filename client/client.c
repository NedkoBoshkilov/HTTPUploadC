#include <stdio.h>
#include "http_uploader.h"

int main(int argc, char** argv)
{
    int result = 0;
    if(argc < 2)
    {
        printf("Usage: %s [file]\n", argv[0]);
        return 0;
    }

    setSeed(4897465, 56168745, 4848749);

    result = uploadFile(argv[1], "file", "127.0.0.1", 8000, "/");

    printf("Result %d\n", result);

    return 0;
}