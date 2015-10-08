/*
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
*/

#include "mfs.h"

#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <stdarg.h>

#include "mfslib.h"




static void m_usage (void)
  {
    fprintf (stderr, "usage:  mfs [FUSE and mount options] rootDir mountPoint\n");
    abort ();
  }

static FILE * log_open (void)
  {
    FILE * logfile;

    // very first thing, open up the logfile and mark that we got in
    // here.  If we can't open the logfile, we're dead.
    logfile = fopen ("mfs.log", "w");
    if (logfile == NULL)
      {
        perror ("logfile");
        exit (EXIT_FAILURE);
      }

    // set logfile to line buffering
    setvbuf (logfile, NULL, _IOLBF, 0);

    return logfile;
  }

void log_msg (const char * format, ...)
  {   
    va_list ap;
    va_start (ap, format);

    vfprintf (M_DATA -> logfile, format, ap);
  }   

#if 0
//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void m_fullpath (char fpath [PATH_MAX], const char * path)
  {
#if 0
    strcpy  (fpath, M_DATA -> rootdir);
    strncat (fpath, path, PATH_MAX); // ridiculously long paths will
                                            // break here

    log_msg ("    m_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
             M_DATA->rootdir, path, fpath);
#else
    strcpy  (fpath, "");
    strncat (fpath, path, PATH_MAX); // ridiculously long paths will
                                            // break here

    log_msg ("    m_fullpath:  rootdir = \"%s\", path = \"%s\", fpath = \"%s\"\n",
             "", path, fpath);
#endif
  }
#endif

static time_t m2uTime (word36 mtime)
  {
     // Convert from fscom format to uSecs since 1901-01-01
     word72 lmtime = ((word72) mtime) << 16;
    // Convert to seconds since 1901-01-01
    lmtime /= 1000000lu;
    // Convert Multics uSecs since 1901-01-01 to UNIX uSecs since 1970-01-01
    lmtime -= 2177452800lu;
    // Add the 22 year Y@K Fudge
    lmtime += (1438644783lu - 744420783lu);
    time_t utime = (time_t) lmtime;
    return utime;
  }

static int find_uid (struct m_state * m_data, word36 uid)
  {
    int vtoc_cnt = m_data -> vtoc_cnt;
    struct vtoc * vtoc = m_data -> vtoc;
    for (int i = 0; i < vtoc_cnt; i ++)
      if (vtoc [i] . uid == uid)
        return i;
    return -1;
  }

static int m_readdir (const char * path, void * buf, 
                      fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info * fi)
  {
    struct stat st;
    memset (& st, 0, sizeof (st));
    int ind = mx_lookup_path (path);
    if (ind < 0)
      {
        printf ("mx_lookup_path of %s failed\n", path);
        return -1;
      }   
    struct m_state * m_data = M_DATA;

next:;
    struct vtoc * vtocp = m_data -> vtoc + ind;
    struct entry * entryp = vtocp -> entries;

    if (offset >= vtocp -> ent_cnt)
      return 0;

    if (entryp [offset] . type == 7 || // segment
        entryp [offset] . type == 4) // directory
      {
        int pri_ind = find_uid (m_data, entryp [offset] . uid);
        if (pri_ind < 0)
          {
            printf ("find_uid failed; uid %012lo %s\n", entryp [offset] . uid, entryp [offset] . name);
            printf ("  path %s\n", path);
            printf ("  ind %d\n", ind);
            printf ("  offset %ld\n", offset);
            printf ("  ent_cnt %d\n", vtocp -> ent_cnt);
            printf ("  type %d\n", entryp [offset] . type);

            return -1;
          }
        st . st_mode = 0444;
        st . st_mtime = m2uTime (m_data -> vtoc [pri_ind] . dtm);
        st . st_atime = m2uTime (m_data -> vtoc [pri_ind] . dtu);
        st . st_ctime = m2uTime (m_data -> vtoc [pri_ind] . time_created);
        int f;
        if (strcmp (">", entryp [offset] . name) == 0)
            f = filler (buf, "/", & st, offset + 1);
        else
            f = filler (buf, entryp [offset] . name, & st, offset + 1);
        if (f == 0)
          {
            offset ++;
            goto next;
          }
      }
    else if (entryp [offset] . type == 5) // link
      {
        st . st_mode = 0444;
        st . st_mtime = m2uTime (m_data -> vtoc [ind] . dtm);
        st . st_atime = m2uTime (m_data -> vtoc [ind] . dtu);
        st . st_ctime = m2uTime (m_data -> vtoc [ind] . time_created);
        int f;
        if (strcmp (">", entryp [offset] . name) == 0)
            f = filler (buf, "/", & st, offset + 1);
        else
            f = filler (buf, entryp [offset] . name, & st, offset + 1);
        if (f == 0)
          {
            offset ++;
            goto next;
          }
      }

    return 0;
  }

static int m_getattr (const char * path, struct stat * statbuf)
  {
    memset (statbuf, 0, sizeof (struct stat));
// find the directory path
    char s [strlen (path) + 1];
    strcpy (s, path);
    char * last = strrchr (s, '/');
    if (! last)
      {
        printf ("m_getattr no path in %s\n", path);
        return -ENOENT;
      }
    if (s == last) // only root
      strcpy (s, "/");
    else
      * last = 0;
    int dind = mx_lookup_path (s);
    if (dind < 0)
      {
        printf ("m_getattr can't find [%s] [%s]\n", s, path);
        return -ENOENT;
      }
    if (! (M_DATA -> vtoc [dind] . attr & 0400000))
      {
        printf ("m_getattr dir not dir? [%s]\n", path);
        return -ENOENT;
      }


    int ind = mx_lookup_path (path);
    if (ind < 0)
      return -ENOENT;
    char * basename = strrchr (path, '/');
    if (! basename)
      {
        printf ("m_getattr no basename in %s\n", path);
        return -ENOENT;
      }
    basename ++;
    if (strlen (basename) == 0)
      { // ? getattr of a directory
        if (! (M_DATA -> vtoc [ind] . attr & 0400000))
          {
            printf ("getattr dir not dir? %s\n", path);
            return -ENOENT;
          }
        statbuf ->  st_mtime = m2uTime (M_DATA -> vtoc [ind] . dtm);
        statbuf ->  st_atime = m2uTime (M_DATA -> vtoc [ind] . dtu);
        statbuf ->  st_ctime = m2uTime (M_DATA -> vtoc [ind] . time_created);
        statbuf -> st_mode = S_IFDIR | 0555;
        statbuf -> st_nlink = 1;
        statbuf -> st_size = 0;
        return 0;
      }
//log_msg ("getattr lookup of %s found %s\n", path, M_DATA -> vtoc [ind] . fq_name);

// find basename in entries

    struct entry * entryp = M_DATA -> vtoc [dind] . entries;
    int ent_cnt = M_DATA -> vtoc [dind] . ent_cnt;
    int eind;
    for (eind = 0; eind < ent_cnt; eind ++)
      if (strcmp (basename, entryp [eind] . name) == 0)
        break;
    if (eind >= ent_cnt)
      {
        printf ("getattr can't find %s in entries\n", basename);
        return -ENOENT;
      }
     
    if (entryp [eind] . type == 5) // link
      {
        // XXX link times?
        statbuf ->  st_mtime = 0;
        statbuf ->  st_atime = 0;
        statbuf ->  st_ctime = 0;
        statbuf -> st_mode = S_IFLNK | 0444;
        statbuf -> st_nlink = 1;
        statbuf -> st_size = 0;
        return 0;
      }

    int pri_ind = find_uid (M_DATA, entryp [eind] . uid);
    if (pri_ind < 0)
      {
        printf ("find_uid failed; uid %012lo %s\n", entryp [eind] . uid, entryp [eind] . name);
        printf ("  path %s\n", path);
        printf ("  ind %d\n", ind);
        printf ("  eind %d\n", eind);
        //printf ("  ent_cnt %d\n", vtocp -> ent_cnt);
        printf ("  type %d\n", entryp [eind] . type);

        return -1;
      }
    statbuf ->  st_mode = 0444;
    statbuf ->  st_mtime = m2uTime (M_DATA -> vtoc [pri_ind] . dtm);
    statbuf ->  st_atime = m2uTime (M_DATA -> vtoc [pri_ind] . dtu);
    statbuf ->  st_ctime = m2uTime (M_DATA -> vtoc [pri_ind] . time_created);
    if (strcmp (path, "/") == 0)
      {
        statbuf -> st_mode = S_IFDIR | 0555;
        statbuf -> st_nlink = 2;
      }
    else
      {
        if (M_DATA -> vtoc [pri_ind] . attr & 0400000)
          {
            statbuf -> st_mode = S_IFDIR | 0555;
            statbuf -> st_nlink = 1;
            statbuf -> st_size = 0;
          }
        else
          {
            statbuf -> st_mode = S_IFREG | 0444;
            statbuf -> st_nlink = 1;
            statbuf -> st_size = (entryp [eind] . bitcnt + 7) / 8;
          }
      }
    return 0;

#if 0
// ignored https://sourceforge.net/p/fuse/wiki/Getattr%28%29/
    // statbuf -> st_dev = 0;//dev_t     st_dev;     /* ID of device containing file */
// ignored https://sourceforge.net/p/fuse/wiki/Getattr%28%29/
    //statbuf -> st_ino = 0;//ino_t     st_ino;     /* inode number */
    statbuf -> st_mode = 0444;//mode_t    st_mode;    /* protection */
    statbuf -> st_nlink = 1;//nlink_t   st_nlink;   /* number of hard links */
    statbuf -> st_uid = getuid ();//uid_t     st_uid;     /* user ID of owner */
    statbuf -> st_gid = getgid ();//gid_t     st_gid;     /* group ID of owner */
    statbuf -> st_rdev = 0;//dev_t     st_rdev;    /* device ID (if special file) */
    statbuf -> st_size = 0;//off_t     st_size;    /* total size, in bytes */
// ignored https://sourceforge.net/p/fuse/wiki/Getattr%28%29/
    //statbuf -> st_blksize = 0;//blksize_t st_blksize; /* blocksize for file system I/O */
    statbuf -> st_blocks = 0;//blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
    statbuf -> st_atime = 0;//time_t    st_atime;   /* time of last access */
    statbuf -> st_mtime = 0;//time_t    st_mtime;   /* time of last modification */
    statbuf -> st_ctime = 0;//time_t    st_ctime;   /* time of last status change */
    return 0;
#endif
  }

struct fuse_operations m_oper =
 {
    .getattr = m_getattr,
//    .readlink = m_readlink,
//    // no .getdir -- that's deprecated
//    .getdir = NULL,
//    .mknod = m_mknod,
//    .mkdir = m_mkdir,
//    .unlink = m_unlink,
//    .rmdir = m_rmdir,
//    .symlink = m_symlink,
//    .rename = m_rename,
//    .link = m_link,
//    .chmod = m_chmod,
//    .chown = m_chown,
//    .truncate = m_truncate,
//    .utime = m_utime,
//    .open = m_open,
//    .read = m_read,
//    .write = m_write,
//    /** Just a placeholder, don't set */ // huh???
//    .statfs = m_statfs,
//    .flush = m_flush,
//    .release = m_release,
//    .fsync = m_fsync,
//    .setxattr = m_setxattr,
//    .getxattr = m_getxattr,
//    .listxattr = m_listxattr,
//    .removexattr = m_removexattr,
//    .opendir = m_opendir,
    .readdir = m_readdir,
//    .releasedir = m_releasedir,
//    .fsyncdir = m_fsyncdir,
//    .init = m_init,
//    .destroy = m_destroy,
//    .access = m_access,
//    .create = m_create,
//    .ftruncate = m_ftruncate,
//    .fgetattr = m_fgetattr
  };

int main (int argc, char * argv [])
  {
    int fuse_stat;
    struct m_state * m_data;

    if ((getuid () == 0) || (geteuid () == 0))
      {
        fprintf (stderr, "Running MFS as root opens unnacceptable security holes\n");
        return 1;
      }

    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv [argc - 2] [0] == '-') || (argv [argc -1] [0] == '-'))
      m_usage ();

    m_data = malloc (sizeof (struct m_state));
    if (m_data == NULL)
      {
        perror ("m_data malloc");
        abort ();
      }
    memset (m_data, 0, sizeof (struct m_state));

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    m_data -> dsknam = realpath (argv [argc - 2], NULL);
    argv [argc - 2] = argv [argc - 1];
    argv [argc - 1] = NULL;
    argc --;

    m_data -> logfile = log_open ();

    mx_mount (m_data);
    umask (0);
    fuse_stat = fuse_main (argc, argv, & m_oper, m_data);

    return fuse_stat;
}


