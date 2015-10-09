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
typedef uint32_t word24;
typedef uint64_t word36;
typedef unsigned int uint;
typedef __uint128_t word72;

struct entry
  {
    char * name;
    word36 uid;
    word24 bitcnt;
// 4 directory
// 5 link
// 7 segment
    int type;
    char * link_target;
    int pri_ind;
  };

struct m_state
  {
    FILE * logfile;
    char * dsknam;
    int fd;
    struct vtoc
      {
        word36 uid;   
        char * name;
        char * dir_name;
        char * fq_name;
        word36 attr;
        time_t dtu;
        time_t dtm;
        time_t time_created;
        int sv;
        int vtoce;
        int32_t filemap [256];
// directory
        int seg_cnt;
        int dir_cnt;
        int lnk_cnt;
        int ent_cnt;
        struct entry * entries;
      } * vtoc;

    int vtoc_no [3];
    int total_vtoc_no;
    int vtoc_cnt;
  };

#define M_DATA ((struct m_state *) fuse_get_context () -> private_data)

void log_msg (const char * format, ...)  __attribute__ ((format (printf, 1, 2)));
