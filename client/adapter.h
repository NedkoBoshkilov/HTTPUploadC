#include "stddef.h"

/* Configure HTTP or HTTPS */
int configure_shttp (const char *_com_port);

/* POST a file */
int post_file_shttp (const char *_com_port, char *_filepath, char *_url,
		     int _content_type, int _timeout, int _post_timeout);

/* GET a file */
int get_file_shttp (const char *_com_port, char *_filepath, char *_url,
		    size_t * _filesize, int _timeout, int _post_timeout);
