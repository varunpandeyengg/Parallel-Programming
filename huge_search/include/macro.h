#ifndef _MACROS_
#define _MACROS_

/* Get Opt Options */
#define OPT "b:f:p:t:h"

/* We only require read access */
#define OPN_MODE "r"

/* Default Size */
#define MIN_BUF 64//256//610155

#define THREAD_MAX 8

#define MAGIC 16
#define BYTE char

#define RADIX 256//3//128//256
#define PRIME 3355439 //Prime Number
#define BILLION 1000000000

#define USAGE(p) fprintf (stderr, "\nUsage: %s -f file -p pattern [-t #thread -b buffer size]", p);

#define LOG_ERR(str) {\
	char buff[20]; \
    time_t now = time(0); \
	struct tm *sTm = gmtime (&now); \
	strftime (buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", sTm); \
    fprintf (stderr, "\n[%s] %s:%s %s", buff, __FILE__, __func__, str); \
}


#define EPRINT(str) fprintf (stderr, str);

#endif //_MACROS_
