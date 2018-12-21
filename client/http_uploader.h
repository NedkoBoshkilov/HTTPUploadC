#ifndef HTTP_UPLOADER_H_
#define HTTP_UPLOADER_H_

#include <stdint.h>

/*
 * Uploads a file to a remote host using HTTP POST request
 * 
 * _filepath - pointer to a string containing the full path to the file
 * _form_name - pointer to a string containing the name of the form field to which the file will be uploaded
 * _host - pointer to a string containing the IPv4 address of the remote host in dot-decimal notation
 * _port - the port number of the remote host
 * _uri - pointer to a string containing the remote resourse address after the host and port parts
 */
int upload_file (const char *_filepath, const char *_form_name,
		 const char *_host, uint16_t _port, const char *_uri);

/*
 * Sets the seeding values for the random number generator
 * 
 * _start_value - the starting value of the RNG
 * _mult_part - the multiplicative part of the RNG (should be x * 4 + 1 or else will be rounded down)
 * _add_part - the additive part of the RNG (should be odd or will be rounded up)
 */
void set_seed (uint32_t _start_value, uint32_t _mult_part, uint32_t _add_part);

/*
 * Generates the next random number and returns it
 */
uint32_t generate_number ();

#endif // HTTP_UPLOADER_H_
