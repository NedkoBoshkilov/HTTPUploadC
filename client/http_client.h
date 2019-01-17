#ifndef HTTP_UPLOADER_H_
#define HTTP_UPLOADER_H_

#include <stdint.h>

/*
 * Uploads a file to a remote host using HTTP POST request
 * 
 * com_port - port over which all comunications are happening
 * filepath - pointer to a string containing the full path to the file
 * form_name - pointer to a string containing the name of the form field to
 * 		which the file will be uploaded
 * host - pointer to a string containing the IPv4 address of the remote host in
 * 		dot-decimal notation
 * port - pointer to a string containing the port number of the remote host
 * uri - pointer to a string containing the remote resourse address after the
 * 		host and port parts
 * next_location - pointer where the next location string will be stored if a
 * 		redirection is present
 * content_type - sets content-type header setting and data format sending
 * com_ms - time to wait for the device to respond to a command in ms
 * http_ms - time to wait for the remote server to respond in ms
 */
int upload_file (const char *_com_port, const char *_filepath,
		 const char *_form_name, const char *_host, const char *_port,
		 const char *_uri, char *_next_location, int _content_type,
		 int _com_ms, int _http_ms);

/*
 * Gets a remote resource using HTTP GET request
 * 
 * com_port - port over which all communications are happening
 * filepath - pointer to a string containing the full path to the file where the
 * 		data will be saved
 * filesize - pointer to a uint32_t where the file's size in bytes will be
 * 		stored - can be NULL
 * host - pointer to a string containing the IPv4 address of the remote host in
 * 		dot-decimal notation
 * port - pointer to a string containing the port number of the remote host
 * uri - pointer to a string containing the remote resourse address after the
 * 		host and port parts
 * next_location - pointer where the next location string will be stored if a
 * 		redirection is present
 * com_ms - time to wait for the device to respond to a command in ms
 * http_ms - time to wait for the remote server to respond in ms
 */
int get_resource (const char *_com_port, const char *_filepath,
		  uint32_t *_filesize, const char *_host, const char *_port,
		  const char *_uri, char *_next_location, int _com_ms,
		  int _http_ms);

/*
 * Sets the seeding values for the random number generator
 * 
 * start_value - the starting value of the RNG
 * mult_part - the multiplicative part of the RNG (should be x * 4 + 1 or else
 * 		will be rounded down)
 * add_part - the additive part of the RNG (should be odd or else will be
 * 		rounded up)
 */
void set_seed (uint32_t _start_value, uint32_t _mult_part, uint32_t _add_part);

/*
 * Generates the next random number and returns it
 */
uint32_t generate_number ();

#endif // HTTP_UPLOADER_H_
