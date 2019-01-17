#include <stdbool.h>
#include <string.h>

#include "adapter.h"
#include "http_client.h"

#define URI_SIZE 256
#define HOST_SIZE 64

static int parse_host (const char *_host_port_combo, char *_host, char *_port);
static int build_uri (char *_uri, const char *_resource,
		      const char *_parameters);
static int calculate_redirect (char *_next_location, char *_host, char *_port,
			       const char **_resource, char *empty);

static int
parse_host (const char *host_port_combo, char *host, char *port) {
	const char *temp;

	if (NULL == host_port_combo) {
		return -1;
	}

	if (NULL == host) {
		return -1;
	}

	if (NULL == port) {
		return -1;
	}

	temp = strchr (host_port_combo, ':');
	/* If Port is specified */
	if (NULL != temp) {
		strncpy (host, host_port_combo, temp - host_port_combo);
		host[temp - host_port_combo] = '\0';
		strcpy (port, temp + 1);
	} else {
		strcpy (host, host_port_combo);
		strcpy (port, "80");
	}

	return 0;
}

static int
build_uri (char *uri, const char *resource, const char *parameters) {
	const char parameters_prefix[] = "";

	if (NULL == uri) {
		return -1;
	}
	if (NULL == resource) {
		return -1;
	}
	if (NULL == parameters) {
		return -1;
	}

	uri[0] = '\0';
	if ('/' != resource[0]) {
		uri[0] = '/';
		uri[1] = '\0';
	}
	strcat (uri, resource);
	if ((strlen (parameters) > 0) && (NULL == strchr (uri, '?'))) {
		strcat (uri, parameters_prefix);
		strcat (uri, parameters);
	}
	return 0;
}

static int
calculate_redirect (char *next_location, char *host, char *port,
		    const char **resource, char *empty) {
	char *host_begin;
	char *temp;

	if (NULL == next_location) {
		return -1;
	}
	if (NULL == host) {
		return -1;
	}
	if (NULL == port) {
		return -1;
	}
	if (NULL == resource) {
		return -1;
	}

	if ('h' == next_location[0]) {
		/* http[s]://host[:port]/resource */
		host_begin = strchr (next_location, ':') + 3;
		temp = strchr (host_begin, '/');

		/* If something after host */
		if (NULL != temp) {
			temp[0] = '\0';
			++temp;
		} else {
			temp = empty;
		}

		*resource = temp;
		if (0 != parse_host (host_begin, host, port)) {
			return -1;
		}
	} else if ('/' == next_location[0]) {
		/* /resource */
		*resource = next_location;
	} else {
		return -1;
	}
	return 0;
}

/* Header Functions */
int
configure_shttp (const char *com_port) {
	return 0;
}

int
post_file_shttp (const char *com_port, const char *filepath,
		 const char *host_begin, const char *resource,
		 const char *parameters, int content_type, int timeout,
		 int post_timeout) {
	int result;
	char current_host[HOST_SIZE];
	char current_port[6];
	bool found;
	char current_uri[URI_SIZE];
	char next_location[URI_SIZE];
	char empty = '\0';

	if (NULL == com_port) {
		return -1;
	}
	if (NULL == filepath) {
		return -1;
	}
	if (NULL == host_begin) {
		return -1;
	}
	if (NULL == resource) {
		return -1;
	}
	if (NULL == parameters) {
		return -1;
	}

	result = parse_host (host_begin, current_host, current_port);
	if (0 != result) {
		return -1;
	}

	found = false;
	while (!found) {
		result = build_uri (current_uri, resource, parameters);
		if (0 != result) {
			return -1;
		}

		next_location[0] = '\0';
		result =
		  upload_file (com_port, filepath, NULL, current_host,
			       current_port, current_uri, next_location,
			       content_type, post_timeout, timeout);

		/* If moved */
		if ((result > 0) && (3 == (result / 100))) {
			if (0 !=
			    calculate_redirect (next_location, current_host,
						current_port, &resource,
						&empty)) {
				return -1;
			}
		} else {	/* Error or success */

			found = true;
		}
	}
	return result;
}

int
get_file_shttp (const char *com_port, const char *filepath,
		const char *host_begin, const char *resource,
		const char *parameters, size_t * filesize, int timeout,
		int get_timeout) {
	int result;
	char current_host[HOST_SIZE];
	char current_port[6];
	bool found;
	char current_uri[URI_SIZE];
	char next_location[URI_SIZE];
	char empty = '\0';
	uint32_t filesize_local;

	if (NULL == com_port) {
		return -1;
	}
	if (NULL == filepath) {
		return -1;
	}
	if (NULL == host_begin) {
		return -1;
	}
	if (NULL == resource) {
		return -1;
	}
	if (NULL == parameters) {
		return -1;
	}

	result = parse_host (host_begin, current_host, current_port);
	if (0 != result) {
		return -1;
	}

	found = false;
	while (!found) {
		result = build_uri (current_uri, resource, parameters);
		if (0 != result) {
			return -1;
		}

		next_location[0] = '\0';
		result =
		  get_resource (com_port, filepath, &filesize_local,
				current_host, current_port, current_uri,
				next_location, get_timeout, timeout);

		/* If moved */
		if ((result > 0) && (3 == (result / 100))) {
			if (0 !=
			    calculate_redirect (next_location, current_host,
						current_port, &resource,
						&empty)) {
				return -1;
			}
		} else {	/* Error or success */

			found = true;
			if (NULL != filesize) {
				*filesize = filesize_local;
			}
		}
	}
	return result;
}
