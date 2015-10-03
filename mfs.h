/*
  There are a couple of symbols that need to be #defined before
  #including all the headers.
*/


// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

#include <stdint.h>
#include <stdio.h>
#include <fuse.h>

typedef uint16_t word9;
typedef uint32_t word18;
typedef uint64_t word36;
typedef unsigned int uint;
typedef __uint128_t word72;

struct m_state
  {
    FILE * logfile;
    char * dsknam;
    word36 * uid_table;   
    char * * name_table;
    char * * fq_name_table;
    word36 * attr_table;
    int vtoc_no;
  };

#define M_DATA ((struct m_state *) fuse_get_context () -> private_data)

void log_msg (const char * format, ...)  __attribute__ ((format (printf, 1, 2)));
