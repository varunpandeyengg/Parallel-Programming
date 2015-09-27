#ifndef _MACROS_
#define _MACROS_

/* Get Opt Options */
#define OPT "b:f:p:h"

/* We only require read access */
#define OPN_MODE "r"

/* Default Size */
#define MIN_BUF 256

#define MAGIC 16

#define USAGE(p) fprintf (stderr, "\nUsage: %s -f file -p pattern [-b buffer size] \n", p);

#define LOG_ERR(str) {\
    time_t now = time(NULL); \
    fprintf (stderr, "\n[%s] %s:%s %s\n", ctime (&now), __FILE__, __func__, str); \
}


#define EPRINT(str) fprintf (stderr, str);



#endif //_MACROS_
