#include <string.h>

#include "adapter.h"
#include "http_client.h"

static int chop_url (char *_url, char **_host, char *_port, char **_uri);

static int
chop_url (char *url, char **host, char *port, char **uri) {
	char *temp;

	if (NULL == url) {
		return -1;
	}
	if (NULL == port) {
		return -1;
	}

	if (0 != strncmp (url, "http", 4)) {
		return -1;
	}

	/* Set host beginning */
	*host = strchr (url, ':') + 3;
	/* URI beginning */
	*uri = strchr (*host, '/');
	/* Find host and port delimiter and turn it into \0 terminator */
	temp = strchr (*host, ':');
	*temp = '\0';
	/* Port beginning */
	++temp;
	/* Copy the port string and put \0 terminator */
	strncpy (port, temp, *uri - temp);
	port[*uri - temp] = '\0';
	return 0;
}

int
configure_shttp (const char *com_port) {
	return 0;
}

int
post_file_shttp (const char *com_port, char *filepath, char *url,
		 int content_type, int timeout, int post_timeout) {
	int result;
	char *host;
	char port[6];
	char *uri;

	if (NULL == com_port) {
		return -1;
	}
	if (NULL == filepath) {
		return -1;
	}
	if (NULL == url) {
		return -1;
	}

	result = chop_url (url, &host, port, &uri);
	if (0 == result) {
		result =
		  upload_file (com_port, filepath, NULL, host, port, uri,
			       content_type, post_timeout, timeout);
	}
	*(strchr (host, '\0')) = ':';
	return result;
}

int
get_file_shttp (const char *com_port, char *filepath, char *url,
		size_t * filesize, int timeout, int get_timeout) {
	int result;
	char *host;
	char port[6];
	char *uri;
	uint32_t filesize_local;

	if (NULL == com_port) {
		return -1;
	}
	if (NULL == filepath) {
		return -1;
	}
	if (NULL == url) {
		return -1;
	}

	result = chop_url (url, &host, port, &uri);
	if (0 == result) {
		result =
		  get_resource (com_port, filepath, &filesize_local, host, port,
				uri, get_timeout, timeout);
		if (NULL != filesize) {
			*filesize = filesize_local;
		}
	}
	*(strchr (host, '\0')) = ':';
	return result;
}
