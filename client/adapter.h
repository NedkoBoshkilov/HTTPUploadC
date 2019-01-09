#include "stddef.h"

/* Configure HTTP or HTTPS */
int configure_shttp (const char *_com_port);

/* Configure content-type header */
int configure_shttp_conttype (int _content_type, const char *_com_port);

/* Set URL */
int set_url_shttp (char *_url, int _timeout, const char *_com_port);

/* POST a file */
int post_file_shttp (char *_filepath, int _timeout, int _post_timeout,
		     const char *_com_port);

/* GET a file */
int get_file_shttp (char *_filepath, size_t *_filesize, int _timeout,
		    int _get_timeout, const char *_com_port);
