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

static char* file = NULL;
static size_t ptrn_sz = 0;
static off_t file_size = 0;
static int64_t ptrn_fngrprnt = 0;
static uint16_t thrd_cnt = 0;
static size_t buf_sz = MIN_BUF;
static uint64_t msb_multiplier = 1;

static struct timespec start_ts = {0};
static struct timespec end_ts = {0};

/* An array of that stores the number of newlines per thread */
typedef struct _at_t {
	size_t line;
	_at_t* next; 
} at_t, *at_p;

typedef struct {
	size_t lf_cnt;
	at_p first;
} result_t;

static result_t results [THREAD_MAX] = {0};

//static size_t line_pr_thrd [THREAD_MAX] = {0};
void print_diff(timespec start, timespec end)
{
	timespec tmp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		tmp.tv_sec = end.tv_sec-start.tv_sec-1;
		tmp.tv_nsec = BILLION+end.tv_nsec-start.tv_nsec;
	} else {
		tmp.tv_sec = end.tv_sec-start.tv_sec;
		tmp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}

	fprintf (stderr, "\nTotal time taken: %ldsec %ldnsec", tmp.tv_sec, tmp.tv_nsec);
}
/* Cleanup rutine before successfull termination of this program
 */
void OnExit (void) 
{	
	if (file) free (file);

	size_t total_lf = 0;
	for (int id = 0; id < THREAD_MAX; ++id) {
		at_p at = results[id].first;
		while (at) {
			at_p tmp = at;
			fprintf (stderr, "\nPattern Found at line: %ld", total_lf + at->line);
			at = at->next;
			free (tmp);
		}
		
		total_lf += results[id].lf_cnt;
		total_lf++; //Minor Correction
	}
	
	//print_diff(start_ts, end_ts);
	EPRINT ("\nTerminating Now...\n");
}

/* Assuing that the bytes is in the msb position, removes 
 * the byte.
 * returns 0 on success
 */
int remove_byte(int64_t& current_digest, BYTE from_digest)
{
		int ret_val = 0;
//fprintf (stderr, "\nByte Removed %d", from_digest);
		current_digest = current_digest - (msb_multiplier * from_digest); //shift the byte
		/*After much head banging, I am adding this code. Underflowing hash, damn it!*/
		while (current_digest < 0)
			current_digest += PRIME;
			
		return ret_val;
}

/* Insert single byte into the digest 
 * Note: - The first byte will goto msb position. So data[0]
 * is at the highest order of the polynomial. Subsequently,
 * this is the first byte to be kicked out of our window.
 * Window size = pattern size (for now). For large patterns
 * proportional to text, window size can be controlled.
 * returns 0 on success
 *
 */
int insert_byte(int64_t& current_digest, BYTE to_digest, intptr_t thrd_id)
{
	int ret_val = 0;

//fprintf (stderr, "\nByte Addded %d", to_digest);
	current_digest *= RADIX; //shift the byte
	current_digest += to_digest;
	current_digest %= PRIME;

	if (to_digest == '\n' && thrd_id != -1)
		results [thrd_id].lf_cnt++;
	return ret_val;
}

/* Process $size bytes and add it into the into the digest
 * returns 0 on success
 */
int insert_bytes (int64_t& current_digest, BYTE* data, size_t size, intptr_t thrd_id)
{
	int ret_val = 0;
	for (auto i = (size_t)0; i < size; i++){
		insert_byte (current_digest, data[i], thrd_id);		
	}

	return ret_val;
}

void* thread_proc (void* param)
{
	int rc = 0;	
	
	BYTE* buf = NULL;
	BYTE* window = NULL;

	size_t msb = 0;
	FILE* fp = NULL;
	//size_t rd_sz = 0;
	off_t rd_off = 0;
	size_t to_read = 0;
	size_t byts_read = 0;
	int64_t data_dgst = 0;
	
	struct timespec start_ts = {0};
	struct timespec end_ts = {0};


	intptr_t id = (intptr_t) param;
	if (id < 0)
		return (struct thread_info *) NULL;

	to_read = (thrd_cnt == 0) ? file_size : file_size/thrd_cnt;
	rd_off = to_read * id;

//	fprintf (stderr, "\nInside Thraed %ld", id);

	/* No overlapping needed if sigle thread */ 
	if (thrd_cnt)
		to_read += (ptrn_sz - 1);

	if (ptrn_sz > buf_sz) {
		buf_sz = ptrn_sz * MAGIC;
		if (!buf_sz) buf_sz = MIN_BUF; //Just in case...

		fprintf (stderr, "\nBuffer Size = %ld ", buf_sz);
		EPRINT ("[ignoring buffer size. Reason: Size less than pattern itself]\n");
	}
			
	do {
		
		if (!to_read)
			break;

		buf = (BYTE*) malloc (buf_sz);
		window = (BYTE*) malloc (ptrn_sz);
		
		if (!buf) {		
			perror("");
			LOG_ERR ("Malloc Error");
			break;

		}
		bzero (buf, buf_sz); /* Not needed really */
		bzero (window, ptrn_sz);

		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_ts);

		//rd_sz = buf_sz;
		fp = fopen (file, OPN_MODE);
		if (!fp) {
			perror("");
			LOG_ERR ("Unable to open file");
			break;
		}
		
		if (rd_off)
			rc = fseek (fp, rd_off, SEEK_SET);
		
		if (rc != 0) {
			LOG_ERR ("Unable to seek the string.");
			break;
		}
		
//		fprintf (stderr, "\nTotal Bytes to read = %ld from offset %ld", to_read, rd_off);

		byts_read = fread (window, sizeof (char), ptrn_sz, fp);
		if (byts_read != ptrn_sz) {
			LOG_ERR ("Failed while initializing the window...");
		    break;
		} else {
			insert_bytes (data_dgst, window, ptrn_sz, id);
		/*	if (id == 0)
				fprintf (stderr, "\nCurrent window %s - %" PRIi64 "|%" PRIi64, window, data_dgst, ptrn_fngrprnt);
		*/
			/* Check if the signature matches the pattern */
			if (data_dgst == ptrn_fngrprnt) {
				EPRINT ("\nFound It!");
			}	

			/* Trivial Step, can be avoided */
			to_read -= byts_read;
		}
		
		while (to_read > 0) {
			bzero (buf, buf_sz); // TODO, Not needed
			byts_read = fread (buf, sizeof (char), buf_sz, fp);
			if (ferror(fp))
				LOG_ERR ("Error occured while reading the file");

			if (0 == byts_read) {
//				fprintf (stderr, "\nThread %ld We read 0 bytes. Bytes left %ld", id, to_read-byts_read);
				break;
			}
			
			for (size_t i = 0; i < byts_read; i++) {	
				remove_byte (data_dgst, window [msb]);
				window [msb] = *(buf+i);
				insert_byte (data_dgst, *(buf+i), id);

				if ((++msb) == ptrn_sz)
					msb = 0;

/*				if (id == 0)
					fprintf (stderr, "\nCurrent window %s - %" PRIi64 "|%" PRIi64, window, data_dgst, ptrn_fngrprnt);*/
				if (data_dgst == ptrn_fngrprnt) {
//					fprintf (stderr, "\nThread %d Current window %s", id, window);
					at_p nw_loc = (at_p) malloc (sizeof(at_t));
					nw_loc->line = results [id].lf_cnt + 1;
					nw_loc->next = results [id].first;
					results [id].first = nw_loc;
//					EPRINT ("\nFound It!");
//					break;
				}	
			}
			
			if (to_read < buf_sz)
				buf_sz = to_read;
			else
				to_read -= byts_read;
		}

	} while (0);

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_ts);
	print_diff(start_ts, end_ts);

	if (buf) free (buf);
	if (window) free (window);
	if (fp) fclose (fp);

	
//	fprintf (stderr, "\nClosing Thraed %d", id);
	
	return (struct thread_info *) NULL;
}

int main (int argc, char ** argv) 
{
	int opt;
	int rc = 0;
	pthread_t threads [THREAD_MAX];
	struct stat stats = {0};	
	char* pattern = NULL;
	
	atexit (OnExit);

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
		case 't':
			thrd_cnt = atoi (optarg);
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
		if (optind < argc) {
			LOG_ERR ("Unexpected argument after options\n");
            USAGE (argv[0]);
			break;
		}

        if (!file || !pattern) {
            LOG_ERR ("Invalid Arguments!");
            USAGE (argv[0]);
            break;
		}

		if (thrd_cnt < 0 || thrd_cnt > 8) {
            LOG_ERR ("Thread count should be between 1 and 8");
            USAGE (argv[0]);
            break;
		}

		rc = stat (file, &stats);
		if (rc != 0) {	
			perror("");
			LOG_ERR ("Error found while getting stats");
			break;
		}

		file_size = stats.st_size;
		bzero (&results, sizeof(results));

		/* Initialize msb multiplier */
		for (size_t i = 1; i < ptrn_sz; i++)
			msb_multiplier = (msb_multiplier * RADIX) % PRIME;

//fprintf (stderr, "\nMsb Multiplier %" PRIu64, msb_multiplier);

		/* Create the pattern fingerprint */
		insert_bytes (ptrn_fngrprnt, pattern, ptrn_sz, -1);

		/* Preprocessing complete, start the timer */
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_ts);

		/* single run*/
		if (0 == thrd_cnt) {
			nice (19);
			thread_proc ((void*) thrd_cnt);
		} else {
			/* To get a fair chance, lets improve the niceness of this program */
			nice (-20); //Best possible niceness

			for (int i = 0; i < thrd_cnt; ++i) 
			{ 
				rc = pthread_create (&threads[i], NULL, &thread_proc, (void*) i);
				if (0 != rc) {
					perror("");
					LOG_ERR ("Fatal Error! Unable to Spawn the thread");
					break;
				} 
			}

			for (int i = 0; i < thrd_cnt; ++i) 
			{
				rc = pthread_join (threads[i], NULL);	
				if (0 != rc) {
					perror("");
					LOG_ERR ("Fatal Error! Unable to join the thread");
					break;
				}

			}

		}

		/* Search Complete, End the timer */
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_ts);	

	} while (0);

	return rc;
}
