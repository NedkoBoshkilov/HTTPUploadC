#include <getopt.h>
#include <time.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adapter.h"

extern char *optarg;
extern int optind, opterr, optopt;


/*
 * -g Get resource
 * -s Send resource
 * -c Content-Type
 */
static void
usage (char *name) {

	printf
	  ("usage: %s [-g | -s [-c 1|2]] com_filepath http://host:port/uri filepath\n",
	   name);
}

int
main (int argc, char **argv) {
	bool get_flag;
	bool send_flag;
	int content_type;
	char *com_port;
	char *url;
	char *filepath;
	size_t filesize;

	int current_opt;
	int idx;
	int result;

	get_flag = false;
	send_flag = false;
	content_type = 0;
	while ((current_opt = getopt (argc, argv, "gsc:")) != -1) {
		switch (current_opt) {
		case 'g':
			get_flag = true;
			break;

		case 's':
			send_flag = true;
			break;

		case 'c':
			if ((*optarg != '1') && (*optarg != '2')) {
				usage (argv[0]);
				printf ("Incorrect content-type\n");
				exit (1);
			}

			content_type = *optarg - '0';
			break;

		case '?':
			usage (argv[0]);
			exit (1);
		}
	}

	if (get_flag == send_flag) {
		usage (argv[0]);
		printf
		  ("Exatly one of the following options must be set: '-g' '-s'\n");
		exit (1);
	}

	if (optind + 1 > argc) {
		usage (argv[0]);
		printf ("Missing com_filepath\n");
		exit (1);
	}

	com_port = argv[optind];
	if (optind + 2 > argc) {
		usage (argv[0]);
		printf ("Missing URL\n");
		exit (1);
	}
	url = argv[optind + 1];

	if (optind + 3 > argc) {
		usage (argv[0]);
		printf ("Missing filepath\n");
		exit (1);
	}
	filepath = argv[optind + 2];

	if (get_flag) {
		/* GET */
		result =
		  get_file_shttp (com_port, filepath, url, &filesize, 10000,
				  2500);
		if (0 == result) {
			printf ("Get successful\n");
			printf ("Filesize %lu bytes\n", filesize);
		} else {
			if (result < 0) {
				printf ("Error getting. Wrong parameters?\n");
				printf ("COM-File: %s\n", com_port);
				printf ("URL: %s\n", url);
				printf ("Filename: %s\n", filepath);
			} else {
				printf
				  ("Error getting. Server returned code: %d\n",
				   result);
			}
		}
	} else {
		/* Upload */
		if (0 == content_type) {
			usage (argv[0]);
			printf ("Missing content-type\n");
			exit (1);
		}

		result =
		  post_file_shttp (com_port, filepath, url, content_type, 10000,
				   2500);
		if (0 == result) {
			printf ("Upload successful\n");
		} else {
			if (result < 0) {
				printf ("Error uploading. Wrong parameters?\n");
				printf ("COM-File: %s\n", com_port);
				printf ("URL: %s\n", url);
				printf ("Filename: %s\n", filepath);
				printf ("Content-type: %d\n", content_type);
			} else {
				printf
				  ("Error uploading. Server returned code: %d\n",
				   result);
			}
		}
	}

	exit (0);
}
