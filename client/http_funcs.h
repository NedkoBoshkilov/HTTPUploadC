#ifndef HTTP_FUNCTIONS_H_
#define HTTP_FUNCTIONS_H_

#include "http_defines.h"

/*
 * Builds an HTTP request into a string based on the parameters. Payload of the request is not included
 * 
 * _request - pointer to a http_request structure filled with the nessesary parameters for the request.
 * _dest - pointer to where the request string will be stored. It should have sufficient space.
 * _request_size - pointer to a uint32_t where the length of the string will be stored if not NULL.
 */
int build_request (const struct http_request *_request, char *_dest,
		   uint32_t *_request_size);


/*
 * Decodes an HTTP response string into a prepared http_response structure.
 * Preparation is needed when searching for headers' values.
 * 
 * _response - pointer to the response string
 * _dest - pointer to the prepared http_response structure
 * _headers_length - pointer to a uint32_t where the length of the headers will be stored if not NULL
 */
int decode_response (const char *_response,
		     struct http_response *_dest, uint32_t *_headers_length);

/*
 * Builds the payload's internal header into a string when a multipart/form-data upload is used.
 * 
 * _filename - pointer to a string containing the filename of the file
 * _form_name - pointer to a string containing the name of the field in the form to which we upload
 * _boundary - pointer to a string containing the boundary used for separation
 * _header - pointer where the header string will be stored. It should have sufficient space
 * _header_size - pointer to a uint32_t where the length of the header string will be stored if not NULL
 */
int build_upload_payload_header (const char *_filename,
				 const char *_form_name,
				 const char *_boundary,
				 char *_header, uint32_t *_header_size);

/*
 * Builds the payload's internal trailer into a string when a multipart/form-data upload is used.
 * 
 * _boundary - pointer to a string containing the boundary used for separation.
 * _trailer - pointer where the trailer string will be stored. It should have sufficient space.
 * _trailer_size - pointer to a uint32_t where the length of the trailer string will be stored if not NULL.
 */
int build_upload_payload_trailer (const char *_boundary,
				  char *_trailer, uint32_t *_trailer_size);

#endif /* HTTP_FUNCTIONS_H_ */
