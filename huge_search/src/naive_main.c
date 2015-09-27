/*
 * naive_main.c
 * 
 * Created by Varun Pandey on 09-21-2015.
 * 
 * Copyright (c) 2015 Varun Pandey
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the project's author nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "../include/macro.h"
#include "../include/common.h"

int main (int argc, char ** argv) 
{
	int opt;
	int rc = 0;
	char* buf = NULL;
	char* file = NULL;
	size_t buf_sz = MIN_BUF;
	size_t rd_sz = 0;
	size_t offset = 0;
	char* pattern = NULL;
	size_t ptrn_sz = 0;

	while ((opt = getopt(argc, argv, OPT)) != -1)
	{
		switch (opt) 
		{
		case 'f':
			file = (char*) malloc (strlen(optarg) + 1);
			memcpy (file, optarg, strlen(optarg));
			*(file + strlen(optarg)) = 0;
			break;
		case 'b':
			buf_sz = atoi (optarg);
			break;
		case 'p':
			ptrn_sz = strlen(optarg);
			pattern = (char*) malloc (ptrn_sz + 1);
			memcpy (pattern, optarg, ptrn_sz);
			break;
        case 'h':
            USAGE (argv[0]);
            exit (EXIT_SUCCESS);
        default:
            USAGE (argv[0]);
            exit (EXIT_FAILURE);
        }
	}

	do {
        if (!file || !pattern) {
            LOG_ERR ("Invalid Arguments!\n");
            USAGE (argv[0]);
            break;
		}
		
		if (ptrn_sz > buf_sz) {
			/* Default size used*/
			buf_sz = ptrn_sz * MAGIC;
			if (!buf_sz) buf_sz = MIN_BUF; //Just in case...

			/* LOG the behaviorial change */
			fprintf (stderr, "\nBuffer Size = %ld ", buf_sz);
			EPRINT ("[ignoring buffer size. Reason: Size less than pattern itself]\n");
		}
	
		FILE * fp = fopen (file, OPN_MODE);
		if (!fp) {
			perror("");
			LOG_ERR ("Unable to open file");
			break;
		}
		
		buf = (char*) malloc (buf_sz);
		bzero (buf, buf_sz);
		rd_sz = buf_sz;

		while (fread (buf+offset, sizeof (char), rd_sz, fp) != 0) {
#ifdef DEBUG
			EPRINT (buf+offset);
#endif

#ifndef ONCE /* Say hello to epistemology. God I am a nerd ;-) */			
			#define ONCE
			rd_sz = buf_sz-ptrn_sz;
			offset = ptrn_sz-1;
#endif //ONCE

            for (size_t index = 0; index < rd_sz; index++) {
#ifdef DEBUG
				fprintf (stderr, "\nCompairing %.*s v/s %s\n", 
						ptrn_sz, buf+index, pattern);
#endif
				if (0 == memcmp (buf+index, pattern, ptrn_sz)){
					 EPRINT ("\nFound a fit!!!\n");
#ifdef DEBUG /* We break on the first hit */
				exit (EXIT_SUCCESS);
#endif
				}
			}

			memcpy (buf, buf+rd_sz, offset);
			bzero (buf+offset, rd_sz);
		}
	} while (0);

	if (file) free (file);
	if (buf) free (buf);
	return rc;
}
