#include <string.h>

#include "adapter.h"
#include "comms.h"
#include "http_client.h"

/* GLOBALS */
char *host;
char *uri;
uint16_t port;

int
configure_shttp (const char *com_port) {
	return 0;
}

int
configure_shttp_conttype (int content_type, const char *com_port) {
	return set_content_type (content_type);
}


int
set_url_shttp (char *url, int timeout, const char *com_port) {
	char *port_str;
	uint8_t port_length;
	int i;

	host = strchr (url, ':') + 3;
	*(strchr (host, ':')) = '\0';
	port_str = host + strlen (host) + 1;
	uri = strchr (port_str, '/');
	port_length = uri - port_str;

	port = 0;
	for (i = 0; i < port_length; ++i) {
		port *= 10;
		port += port_str[i] - '0';
	}

	return 0;
}

int
post_file_shttp (char *filepath, int timeout, int post_timeout,
		 const char *com_port) {
	int result;

	client_set_timeout (timeout * 1000);
	comms_set_timeout (post_timeout * 1000);
	set_port (com_port);
	result = upload_file (filepath, NULL, host, port, uri);
	set_port (NULL);
	*(strchr (host, '\0')) = ':';
	return result;
}

int
get_file_shttp (char *filepath, size_t *filesize, int timeout, int get_timeout,
		const char *com_port) {
	uint32_t filesize_local;
	int result;

	client_set_timeout (timeout * 1000);
	comms_set_timeout (get_timeout * 1000);
	set_port (com_port);
	result = get_resource (filepath, &filesize_local, host, port, uri);
	set_port (NULL);
	*filesize = filesize_local;
	*(strchr (host, '\0')) = ':';
	return result;
}
