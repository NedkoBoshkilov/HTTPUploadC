#include <getopt.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http_uploader.h"

extern char *optarg;
extern int optind, opterr, optopt;

static void
usage(char *name)
{

    printf("usage: %s [-f form_name] [-h host] [-p port] [-u uri] filepath\n", name);
    exit(1);
}

int
main(int argc, char **argv)
{
    char        form_name_default[5];
    char        host_default[16];
    uint16_t    port_default;
    char        uri_default[2];
    char        *form_name;
    char        *host;
    uint16_t    port;
    char        *uri;
    int current_opt;
    int idx;
    int result;

    strcpy(form_name_default, "file");
    strcpy(host_default, "127.0.0.1");
    port_default = 80;
    strcpy(uri_default, "/");

    form_name = form_name_default;
    host = host_default;
    port = port_default;
    uri = uri_default;

    while ((current_opt = getopt(argc, argv, "f:h:p:u:")) != -1)
    {
        switch(current_opt)
        {
        case 'f':
            form_name = optarg;
            break;
        
        case 'h':
            host = optarg;
            break;
        
        case 'p':
            port = 0;
            for (idx = 0; idx < strlen(optarg); ++idx)
            {
                port *= 10;
                port += optarg[idx] - '0';
            }
            break;

        case 'u':
            uri = optarg;
            break;

        case '?':
            usage(argv[0]);
            exit(1);
        }
    }

    if (optind + 1 > argc)
    {
        usage(argv[0]);
        exit(1);
    }

    set_seed((uint32_t)time(NULL), 56168745, 4848749);
    generate_number();

    result = upload_file(argv[optind], form_name, host, port, uri);
    if (0 == result)
    {
        printf("Upload successful\n");
    } else {
        if (result < 0)
        {
            printf("Error uploading. Wrong parameters?\n");
            printf("Filepath: %s\n", argv[optind]);
            printf("Form name: %s\n", form_name);
            printf("Host: %s\n", host);
            printf("Port: %u\n", port);
            printf("URI: %s\n", uri);
        } else {
            printf("Error uploading. Server returned code: %d\n", result);
        }
    }

    exit(0);
}