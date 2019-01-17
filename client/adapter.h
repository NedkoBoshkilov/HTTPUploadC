#include "stddef.h"

/* Configure HTTP or HTTPS */
int configure_shttp (const char *_com_port);

/* 
 * POST a file
 * 
 * com_port - port to be used for communication
 * filepath - path to file for data to be saved
 * host_begin - host or host:port combo
 * resource - url after host part before parameters
 * parameters - parameters of the url
 * content_type - Content-type of the request
 * timeout - how long to wait remote host
 * get_timeout - how long to wait for command responses
 */
int post_file_shttp (const char *_com_port, const char *_filepath,
		     const char *_host_begin, const char *_resource,
		     const char *_parameters, int _content_type, int _timeout,
		     int _post_timeout);

/* 
 * GET a file
 * 
 * com_port - port to be used for communication
 * filepath - path to file for data to be saved
 * host_begin - host or host:port combo
 * resource - url after host part before parameters
 * parameters - parameters of the url
 * filesize - will be filled with the size of the data - can be NULL
 * timeout - how long to wait remote host
 * get_timeout - how long to wait for command responses
 */
int get_file_shttp (const char *_com_port, const char *_filepath,
		    const char *host_begin, const char *resource,
		    const char *parameters, size_t * _filesize, int _timeout,
		    int _get_timeout);

/*
 * Available content types:
 * 0 - URL_ENCODED (NOT SUPPORTED !!!)
 * 1 - TEXT/PLAIN
 * 2 - APPLICATION/OCTET-STREAM
 * 3 - MULTIPART/FORM-DATA (Needs further implementation)
 */
