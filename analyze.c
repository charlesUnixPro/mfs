
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>

typedef uint16_t word9;
typedef uint32_t word18;
typedef uint32_t word24;
typedef uint64_t word36;
typedef unsigned int uint;
typedef __uint128_t word72;

#define N_SVS 3
// Maximum number of VTOC entries on a sub-volume; unsure XXX
#define N_VTOCW 100000

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
    char * dsknam;
    int fd;
    bool is512;
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

    int vtoc_no [N_SVS];
    int total_vtoc_no;
    int vtoc_cnt;
  };

//#define M_DATA ((struct m_state *) fuse_get_context () -> private_data)

void log_msg (const char * format, ...)  __attribute__ ((format (printf, 1, 2)));
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

//#include "mfslib.h"


// AN61 VM MPM
//   pg 13-1
//  The volume label resides on the first Multics record... It is generated
//  by init_disk_pack_ for non-RPV, init_empty_root for the RPV.
//
//  The label record is deivided into five regions, on sector boundaries:
//
//  1. GCOS, sectors 0 to 4 (label.gcos).
//  2. Permanent region, sector 5 (label.Multics to label.pad1).
//  3. Dynamic information, sector 6 (label,time_mounted to label.pad2).
//  4. Root information, sector 7 (label.root to label.pad3). Only defined
//     on a RPV.
//  5. Partition map sector 10 (octal) (label.parts).
//  The rest of the label record (sectors 11-15 octal) reserved. 

// fs_vol_label.incl.pl1
//
//        dcl  labelp ptr;
//        
//        dcl 1 label based (labelp) aligned,
//        
//        /* First comes data not used by Multics.. for compatibility with GCOS */
//        
//            2 gcos (5*64) fixed bin,
#define label_gcos_sz (5*64)


#define label_perm_os (5*64)
//        
//        /* Now we have the Multics label */
//        
// os sz
//  0  8      2 Multics char (32) init ("Multics Storage System Volume"), /* Identifier */
//  8  1      2 version fixed bin,                                    /* Version 1 */
//  9  8      2 mfg_serial char (32),                                 /* Manufacturer's serial number */
// 17  8      2 pv_name char (32),                                    /* Physical volume name. */
// 25  8      2 lv_name char (32),                                    /* Name of logical volume for pack */
// 33  1      2 pvid bit (36),                                        /* Unique ID of this pack */
// 34  1      2 lvid bit (36),                                        /* unique ID of its logical vol */
// 35  1      2 root_pvid bit (36),                                   /* unique ID of the pack containing the root. everybody must agree. */
// 36  2      2 time_registered fixed bin (71),                       /* time imported to system */
// 38  1      2 n_pv_in_lv fixed bin,                                 /* # phys volumes in logical */
// 39  1      2 vol_size fixed bin,                                   /* total size of volume, in records */
// 40  1      2 vtoc_size fixed bin,                                  /* number of recs in fixed area + vtoc */
// 41  .      2 not_used bit (1) unal,                                /* used to be multiple_class */
//     .      2 private bit (1) unal,                                 /* TRUE if was registered as private */
//     .      2 inconsistent_dbm bit (1) unal,                        /* TRUE if ESD-less crash */
//     1      2 flagpad bit (33) unal,
// 41  2      2 max_access_class bit (72),                            /* Maximum access class for stuff on volume */
// 43  2      2 min_access_class bit (72),                            /* Minimum access class for stuff on volume */
// 45  2      2 password bit (72),                                    /* not yet used */
// 47  1      2 number_of_sv fixed bin,                               /* if = 0 not a subvolume else the number of svs */
// 48  1      2 this_sv fixed bin,                                    /* what subvolume number it is */
// 49  1      2 sub_vol_name char (1),                                /* what subvolume name (a b c d) it is */
// 50 13      2 pad1 (13) fixed bin,
// 64


#define label_dyn_os (6*64)
//  0  2      2 time_mounted fixed bin (71),                          /* time mounted */
//  2  2      2 time_map_updated fixed bin (71),                      /* time vmap known good */
#define label_time_map_updated_os (label_dyn_os + 2)
//        
//        /* The next two words overlay time_unmounted on pre-MR10 systems. This
//           forces a salvage if an MR10 pack is mounted on an earlier system.
//         */
//  4  1      2 volmap_version fixed bin,                             /* version of volume map (currently 1) */
//  5  1      2 pad6 fixed bin,
//        
//  6  2      2 time_salvaged fixed bin (71),                         /* time salvaged */
//  8  2      2 time_of_boot fixed bin (71),                          /* time of last bootload */
// 10  2      2 time_unmounted fixed bin (71),                        /* time unmounted cleanly */
#define label_time_unmounted_os (label_dyn_os + 10)
// 12  1      2 last_pvtx fixed bin,                                  /* pvtx in that PDMAP */
// 13  2      2 pad1a (2) fixed bin,
// 15  1      2 err_hist_size fixed bin,                              /* size of pack error history */
// 16  6      2 time_last_dmp (3) fixed bin (71),                     /* time last completed dump pass started */
// 22  2      2 time_last_reloaded fixed bin (71),                    /* what it says */
// 24 40      2 pad2 (40) fixed bin,
// 64


#define label_root_os (7*64)
//     .      2 root,
//  0  1        3 here bit (1),                                       /* TRUE if the root is on this pack */
//  1  1        3 root_vtocx fixed bin (35),                          /* VTOC index of root, if it is here */
//  2  1        3 shutdown_state fixed bin,                           /* Status of hierarchy */
//  3  1        3 pad7 bit (1) aligned,
//  4  1        3 disk_table_vtocx fixed bin,                         /* VTOC index of disk table on RPV */
//  5  1        3 disk_table_uid bit (36) aligned,                    /* UID of disk table */
//  6  1        3 esd_state fixed bin,                                /* State of esd */
//  7  1      2 volmap_record fixed bin,                              /* Begin record of volume map */
//  8  1      2 size_of_volmap fixed bin,                             /* Number of records in volume map */
//  9  1      2 vtoc_map_record fixed bin,                            /* Begin record of VTOC map */
// 10  1      2 size_of_vtoc_map fixed bin,                           /* Number of records in VTOC map */
// 11  1      2 volmap_unit_size fixed bin,                           /* Number of words per volume map section */
// 12  1      2 vtoc_origin_record fixed bin,                         /* Begin record of VTOC */
#define label_vtoc_origin_record_os (label_root_os + 12)
// 13  1      2 dumper_bit_map_record fixed bin,                      /* Begin record of dumper bit-map */
// 14  1      2 vol_trouble_count fixed bin,                          /* Count of inconsistencies found since salvage */
// 15 52      2 pad3 (52) fixed bin,
// 67 pad3 spills 3 words over in the next sector....
#define label_part_os (8*64)
//  3  1      2 nparts fixed bin,                                     /* Number of special partitions on pack */
//  4         2 parts (47),
//              3 part char (4),                                      /* Name of partition */
//              3 frec fixed bin,                                     /* First record */
//              3 nrec fixed bin,                                     /* Number of records */
//              3 pad5 fixed bin,
//   320      2 pad4 (5*64) fixed bin;
//        
//        dcl  Multics_ID_String char (32) init ("Multics Storage System Volume") static;
//        


// vtoc_header.incl.alm

// "             
// "         Structure vtoc_header
// "             
//           equ       vtoc_header_size,1023
// 
// 
//           equ       vtoc_header.version,0
//           equ       vtoc_header.n_vtoce,1
#define vtoc_header_n_vtoce_os 1
//           equ       vtoc_header.vtoc_last_recno,2
#define vtoc_header_vtoc_last_recno 2
//           equ       vtoc_header.n_free_vtoce,3
#define vtoc_header_n_free_vtoce 3
//           equ       vtoc_header.first_free_vtocx,4
//           equ       vtoc_header.dmpr_bit_map,7


// vtoce.incl.pl1

//        dcl  vtocep ptr;
//        
//        dcl 1 vtoce based (vtocep) aligned,
//        
//        
//  0   1     (2 pad_free_vtoce_chain bit (36),                       /* Used to be pointer to next free VTOCE */
//        
//  1   1     2 uid bit (36),                                         /* segment's uid - zero if vtoce is free */
//        
//  2   .     2 msl bit (9),                                          /* maximum segment length in 1024 word units */
//      .     2 csl bit (9),                                          /* current segment length - in 1024 word units */
//      .     2 records bit (9),                                      /* number of records used by the seg in second storage */
//      1     2 pad2 bit (9),
//        
//  3   1     2 dtu bit (36),                                         /* date and time segment was last used */
//        
//  4   1     2 dtm bit (36),                                         /* date and time segment was last modified */
//        
//  5   .     2 nqsw bit (1),                                         /* no quota switch - no checking for pages of this seg */
//      .     2 deciduous bit (1),                                    /* true if hc_sdw */
//      .     2 nid bit (1),                                          /* no incremental dump switch */
//      .     2 dnzp bit (1),                                         /* Dont null zero pages */
//      .     2 gtpd bit (1),                                         /* Global transparent paging device */
//      .     2 per_process bit (1),                                  /* Per process segment (deleted every bootload) */
//      .     2 damaged bit (1),                                      /* TRUE if contents damaged */
//      .     2 fm_damaged bit (1),                                   /* TRUE if filemap checksum bad */
//      .     2 fm_checksum_valid bit (1),                            /* TRUE if the checksum has been computed */
//      .     2 synchronized bit (1),                                 /* TRUE if this is a data management synchronized segment */
//      .     2 pad3 bit (8),
//      .     2 dirsw bit (1),                                        /* directory switch */
//      .     2 master_dir bit (1),                                   /* master directory - a root for the logical volume */
//      1     2 pad4 bit (16)) unaligned,                             /* not used */
//        
//  6   1     2 fm_checksum bit (36) aligned,                         /* Checksum of used portion of file map */
//        
//  7   1     (2 quota (0:1) fixed bin (18) unsigned,                 /* sec storage quota - (0) for non dir pages */
//        
//  8   1     2 used (0:1) fixed bin (18) unsigned,                   /* sec storage used  - (0) for non dir pages */
//        
//  9   1     2 received (0:1) fixed bin (18) unsigned,               /* total amount of storage this dir has received */
//        
// 10   4     2 trp (0:1) fixed bin (71),                             /* time record product - (0) for non dir pages */
//        
// 14   2     2 trp_time (0:1) bit (36),                              /* time time_record_product was last calculated */
//                         
//        
// 16 128     2 fm (0:255) bit (18),                                  /* file map - 256 entries - 18 bits per entry */
#define vtoce_fm_os 16
//        
// CAC: File map is a table of record #s; high bit on indicates NULL. See null_addresses.incl.pl1
//
//144  10     2 pad6 (10) bit (36),                                   /* not used */
//        
//154   .     2 ncd bit (1),                                          /* no complete dump switch */
//      .     2 pad7 bit (17),
//      1     2 pad8 bit (18),
//        
//155   1     2 dtd bit (36),                                         /* date-time-dumped */
//        
//156   3     2 volid (3) bit (36),                                   /* volume ids of last incremental, consolidated, and complete dumps */
//        
//159   1     2 master_dir_uid bit (36),                              /* superior master directory uid */
//        
//        
//        
//        
//160  16     2 uid_path (0:15) bit (36),                             /* uid pathname of all parents starting after the root */
//        
//176   8     2 primary_name char (32),                               /* primary name of the segment */
#define vtoce_primary_name_os 176
//        
//184   1     2 time_created bit (36),                                /* time the segment was created */
//        
//185   1     2 par_pvid bit (36),                                    /* physical volume id of the parent */
//        
//186   .     2 par_vtocx fixed bin (17),                             /* vtoc entry index of the parent */
//      1     2 branch_rp bit (18)) unaligned,                        /* rel pointer of the branch of this segment */
//        
//187   1     2 cn_salv_time bit (36),                                /* time branch - vtoce connection checked */
//        
//188   2     2 access_class bit (72),                                /* access class in branch */
//190   1     2 perm_flags aligned,
//              3 per_bootload bit (1) unal,                          /* ON => deleted each bootload */
//              3 pad9 bit (35) unal,
//191   1     2 owner bit (36);                                       /* pvid of this volume */
//192 (3*64)

//  
//  fs_dev_types.incl.alm
//  
//  fs_dev.dev_type_names:
//            aci       "bulk"
//            aci       "d500"
//            aci       "d451"
//            aci       "d400"
//            aci       "d190"
//            aci       "d181"
//            aci       "d501"
//            aci       "3380"
//            aci       "3381"
//  
//  fs_dev.sect_per_dev:
//            vfd       36/4000000          Bulk
//            vfd       36/814*40*19        MSU0500
//            vfd       36/814*40*19        MSU0450
//            vfd       36/410*40*19        MSU0400
//            vfd       36/410*31*19        DSU190
//            vfd       36/202*18*20        DSU181
//            vfd       36/840*64*20        MSU0501
//            vfd       36/885*255          FIPS 3380
//            vfd       36/1770*255         FIPS 3381
//  
//  fs_dev.track_per_cyl:
//            vfd       36/1                Bulk
//            vfd       36/19               MSU0500
//            vfd       36/19               MSU0450
//            vfd       36/19               MSU0400
//            vfd       36/19               DSU190
//            vfd       36/20               DSU181
//            vfd       36/20               MSU0501
//            vfd       36/15               FIPS 3380
//            vfd       36/15               FIPS 3381
//  
//  fs_dev.cyl_per_dev:
//            vfd       36/0                Bulk
//            vfd       36/814              MSU0500
//            vfd       36/814              MSU0450
//            vfd       36/410              MSU0400
//            vfd       36/410              DSU190
//            vfd       36/202              DSU181
//            vfd       36/840              MSU0501
//            vfd       36/885              FIPS 3380
//            vfd       36/1770             FIPS 3381
//  
//  fs_dev.sect_per_cyl:
//            vfd       36/4000000          Bulk
//            vfd       36/40*19            MSU0500
//            vfd       36/40*19            MSU0450
//            vfd       36/40*19            MSU0400
//            vfd       36/31*19            DSU190
//            vfd       36/18*20            DSU181
//            vfd       36/64*20            MSU0501
//            vfd       36/255              FIPS 3380
//            vfd       36/255              FIPS 3381
//#define sect_per_cyl 255
//  
//  fs_dev.sect_per_track:
//            vfd       36/1                Bulk
//            vfd       36/40               MSU0500
//            vfd       36/40               MSU0450
//            vfd       36/40               MSU0400
//            vfd       36/31               DSU190
//            vfd       36/18               DSU181
//            vfd       36/64               MSU0501
//            vfd       36/136              FIPS 3380           " 8 * 17 = 136
//            vfd       36/136              FIPS 3381           " 8 * 17 = 136
//  
//  fs_dev.mult_sect_per_cyl:
//            vfd       36/2048*16          Bulk
//            vfd       36/40*19/16*16      MSU0500
//            vfd       36/40*19/16*16      MSU0450
//            vfd       36/40*19/16*16      MSU0400
//            vfd       36/31*19/16*16      DSU190
//            vfd       36/18*20/16*16      DSU181
//            vfd       36/64*20/16*16      MSU0501
//            vfd       36/136*15/16*16     FIPS 3380           " 8 * 17 = 136
//            vfd       36/136*15/16*16     FIPS 3381
//  
//  fs_dev.rem_per_cyl:           " Size of gap
//            vfd       36/0                          Bulk
//            vfd       36/(40*19)-(40*19/16*16)      MSU0500
//            vfd       36/(40*19)-(40*19/16*16)      MSU0450
//            vfd       36/(40*19)-(40*19/16*16)      MSU0400
//            vfd       36/(31*19)-(31*19/16*16)      DSU190
//            vfd       36/(18*20)-(18*20/16*16)      DSU181
//            vfd       36/(64*20)-(64*20/16*16)      MSU0501
//            vfd       36/(136*15)-(136*15/16*16)    FIPS 3380 " 8 * 17 = 136
//            vfd       36/(136*15)-(136*15/16*16)    FIPS 3381 " 8 * 17 = 136
//  
//  fs_dev.rec_per_dev:
//            vfd       36/2048             Bulk       2048.
//            vfd       36/(40*19/16)*814   MSU0500   38258.
//            vfd       36/(40*19/16)*814   MSU0450   38258.
//            vfd       36/(40*19/16)*410   MSU0400   19270.
//            vfd       36/(31*19/16)*410   DSU190    14760.
//            vfd       36/(18*20/16)*202   DSU181     4444.
//            vfd       36/(64*20/16)*840   MSU0501   67200.
//            vfd       36/(136*15/16)*885  FIPS 3380 112395.
//            vfd       36/(136*15/16)*1770 FIPS 3381 224790.
//  
//  fs_dev.amaxio:
//            vfd       36/2048             For bulk store.
//            vfd       36/40*19/16         MSU0500
//            vfd       36/40*19/16         MSU0450
//            vfd       36/40*19/16         MSU0400
//            vfd       36/31*19/16         DSU190
//            vfd       36/18*20/16         DSU181
//            vfd       36/64*20/16         MSU0501
//            vfd       36/136*15/16        FIPS 3380           " 8 * 17 = 136
//            vfd       36/136*15/16        FIPS 3381
//  

// dcl  number_of_sv (9) fixed bin static options (constant) init /* table of subvolumes */
//     (0, 0, 0, 0, 0, 0, 0, 2, 3);
// #define number_of_sv 3
static int number_of_sv;

//  fs_dev_types_sector.incl.pl1
//  
//  dcl  sect_per_cyl (9) fixed bin static options (constant) init /* table of # of sectors per cylinder on each device */
//      (0, 760, 760, 760, 589, 360, 1280, 255, 255);
//#define sect_per_cyl 255
static int sect_per_cyl;
//  
//  dcl  sect_per_sv (9) fixed bin (24) static options (constant) init /* table of # of sectors per cylinder on each subvolume */
//       (0, 0, 0, 0, 0, 0, 0, 112710, 150450);
//  
//  dcl  sect_per_rec (9) fixed bin static options (constant) init
//   /* table of # of sectors per record on each device */
//   /* coresponding array in disk_pack.incl.pl1 called SECTORS_PER_RECORD */
//      (0, 16, 16, 16, 16, 16, 16, 2, 2);
//#define sect_per_rec 2
int sect_per_rec;
//  
//  dcl  sect_per_vtoc (9) fixed bin static options (constant) init
//       (0, 3, 3, 3, 3, 3, 3, 1, 1);
//  
//  dcl  vtoc_per_rec (9) fixed bin  static options  (constant) init
//  /* corespending array in disk_pack.incl.pl1 named VTOCES_PER_RECORD */
//       (0, 5, 5, 5, 5, 5, 5, 2, 2);
//  
//  dcl  sect_per_track (9) fixed bin static options (constant) init /* table of # of sectors per track on each device */
//      (0, 40, 40, 40, 31, 18, 64, 17, 17);
//  
//  dcl  words_per_sect (9) fixed bin static options (constant) init /* table of # of words per sector on each device */
//      (0, 64, 64, 64, 64, 64, 64, 512, 512);
//  


//      /* Template for the directory header. Length = 64 words. */
//      
//      dcl  dp ptr;
//      
//      dcl 1 dir based (dp) aligned,
//      
//  0  1   2 modify bit (36),                                       /* Process ID of last modifier */
//  1  .   2 type bit (18) unaligned,                     /* type of object = dir header */
//     .   2 size fixed bin (17) unaligned,                         /* size of header in words */
//          2 dtc (3),                                              /* date-time checked by salvager array */
//  2  .      3 date bit (36),                                      /* the date */
//     6      3 error bit (36),                                     /* what errors were discovered */
//      
//  8  1    2 uid bit (36),                                         /* uid of the directory           - copied from branch */
//      
//  9  1    2 pvid bit (36),                                        /* phys vol id of the dir         - copied from branch */
//      
// 10  1    2 sons_lvid bit (36),                                   /* log vol id for inf non dir seg - copied from branch */
//      
// 11  2    2 access_class bit (72),                                /* security attributes of dir     - copied from branch */
//      
// 13  .    (2 vtocx fixed bin (17),                                /* vtoc entry index of the dir    - copied from branch */
//     1    2 version_number fixed bin (17),                        /* version number of header */
//      
// 14  .    2 entryfrp bit (18),                                    /* rel ptr to beginning of entry list */
//     1    2 pad2 bit (18),
//      
// 15  .    2 entrybrp bit (18),                                    /* rel ptr to end of entry list */
//     1    2 pad3 bit (18),
//      
// 16  .    2 pers_frp bit (18),                                    /* rel ptr to start of person name list */
//     1    2 proj_frp bit (18),                                    /* rel ptr to start of project name list */
//      
// 17  .    2 pers_brp bit (18),                                    /* rel ptr to end of person name list */
//     1    2 proj_brp bit (18),                                    /* rel ptr to end of project name list */
//      
// 18  .    2 seg_count fixed bin (17),                             /* number of non-directory branches */
//     1    2 dir_count fixed bin (17),                             /* number of directory branches */
//      
// 19  .    2 lcount fixed bin (17),                                /* number of links */
//     1    2 acle_total fixed bin (17),                            /* total number of ACL entries in directory */
//      
// 20  .    2 arearp bit (18),                                      /* relative pointer to beginning of allocation area */
//     .    2 per_process_sw bit (1),                               /* indicates dir contains per process segments */
//     .    2 master_dir bit (1),                                   /* TRUE if this is a master dir */
//     .    2 force_rpv bit (1),                                    /* TRUE if segs must be on RPV */
//     .    2 rehashing bit (1),                                    /* TRUE if hash table is being constructed */
//     1    2 pad4 bit (14),
//      
// 21  .    2 iacl_count (0:7),
//     .      3 seg fixed bin (17),                                 /* number of initial acl entries for segs */
//     8      3 dir fixed bin (17),                                 /* number of initial acl entries for dir */
//    
// 29  .    2 iacl_count (0:7),
//     .      3 seg_frp bit (18),                                   /* rel ptr to start of initial ACL for segs */
//     .      3 seg_brp bit (18),                                   /* rel ptr to end of initial ACL for segs */
//     .
//     .      3 dir_frp bit (18),                                   /* rel ptr to start of initial for dirs */
//    16      3 dir_brp bit (18),                                   /* rel ptr to end of initial ACL for dirs */
//      
// 45  .    2 htsize fixed bin (17),                                /* size of hash table */
//     1    2 hash_table_rp bit (18),                               /* rel ptr to start of hash table */
//      
// 46  .    2 htused fixed bin (17),                                /* no. of used places in hash table */
//     1    2 pad6 fixed bin (17),
//      
// 47  .    2 tree_depth fixed bin (17),                            /* number of levels from root of this dir */
//     1    2 pad7 bit (18)) unaligned,
//      
// 48  1    2 dts bit (36),                                         /* date-time directory last salvaged */
//      
// 49  1    2 master_dir_uid bit (36),                              /* uid of superior master dir */
// 50  1    2 change_pclock fixed bin (35),                         /* up one each call to sum$dirmod */
// 51 11    2 pad8 (11) bit (36),                                   /* pad to make it a 64 word header */
// 62  1    2 checksum bit (36),                                    /* checksummed from uid on */
// 63  1    2 owner bit (36);                                       /* uid of parent dir */
// 64    
//      


//
//      /* Template for an entry. Length = 38 words */
//      
//      dcl  ep ptr;
//      
//      dcl 1 entry based (ep) aligned,
//      
//  0  .    (2 efrp bit (18),                                       /* forward rel ptr to next entry */
//     1    2 ebrp bit (18)) unaligned,                             /* backward rel ptr to previous entry */
//      
//  1  .    2 type bit (18) unaligned,                              /* type of object = dir entry  */
//     1    2 size fixed bin (17) unaligned,                        /* size of dir entry */
//      
//  2  1    2 uid bit (36),                                         /* unique id of entry */
//      
//  3  1    2 dtem bit (36),                                        /* date-time entry modified */
//      
//  4  .    (2 bs bit (1),                                          /* branch switch = 1 if branch */
//     .    2 pad0 bit (17),
//     1    2 nnames fixed bin (17),                                /* number of names for this entry */
//      
//  5  .    2 name_frp bit (18),                                    /* rel pointer to start of name list */
//     1    2 name_brp bit (18),                                    /* rel pointer to end of name list */
//      
//  6  .    2 author,                                               /* user who created branch */
//     .      3 pers_rp bit (18),                                   /* name of user who created branch */
//     1      3 proj_rp bit (18),                                   /* project of user who created branch */
//      
//  7  .      3 tag char (1),                                       /* tag of user who created branch */
//     1      3 pad1 char (3),
//      
//  8 14    2 primary_name bit (504),                               /* first name on name list */
//      
// 22  1    2 dtd bit (36),                                         /* date time dumped */
//      
// 23  1    2 pad2 bit (36),
//      
//      
//      /* the declarations below are for branch only */
//      
//  24  1   2 pvid bit (36),                                        /* physical volume id */
//      
//  25  .   2 vtocx fixed bin (17),                                 /* vtoc entry index */
//      1   2 pad3 bit (18),
//      
//  26  .   2 dirsw bit (1),                                        /* = 1 if this is a directory branch */
//      .   2 oosw bit (1),                                         /* out of service switch  on = 1 */
//      .   2 per_process_sw bit (1),                               /* indicates segment is per process */
//      .   2 copysw bit (1),                                       /* = 1 make copy of segment whenever initiated */
//      .   2 safety_sw bit (1),                                    /* if 1 then entry cannot be deleted */
//      .   2 multiple_class bit (1),                               /* segment has multiple security classes */
//      .   2 audit_flag bit (1),                                   /* segment must be audited for security */
//      .   2 security_oosw bit (1),                                /* security out of service switch */
//      .   2 entrypt_sw bit (1),                                   /* 1 if call limiter is to be enabled */
//      .   2 master_dir bit (1),                                   /* TRUE for master directory */
//      .   2 tpd bit (1),                                          /* TRUE if this segment is never to go on the PD */
//      .   2 pad4 bit (11),
//      1   2 entrypt_bound bit (14)) unaligned,                    /* call limiter */
//      
//  27  2   2 access_class bit (72) aligned,                        /* security attributes : level and category */
//      
//  29  .   (2 ring_brackets (3) bit (3),                           /* ring brackets on segment */
//      .   2 ex_ring_brackets (3) bit (3),                         /* extended ring brackets */
//      1   2 acle_count fixed bin (17),                            /* number of entries on ACL */
//      
//  30  .   2 acl_frp bit (18),                                     /* rel ptr to start of ACL */
//      1   2 acl_brp bit (18),                                     /* rel ptr to end of ACL */
//      
//  31  .   2 bc_author,                                            /* user who last set the bit count */
//      .     3 pers_rp bit (18),                                   /* name of user who set the bit count */
//      1     3 proj_rp bit (18),                                   /* project of user who set the bit count */
//      
//  32  .     3 tag char (1),                                       /* tag of user who set the bit count */
//      .     3 pad5 bit (2),
//      1   2 bc fixed bin (24)) unaligned,                         /* bit count for segs, msf indicator for dirs */
//      
//  33  1   2 sons_lvid bit (36),                                   /* logical volume id for immediat inf non dir seg */
//      
//  34  1   2 pad6 bit (36),
//      
//  35  1   2 checksum bit (36),                                    /* checksum from dtd */
//      
//  36  1   2 owner bit (36);                                       /* uid of containing directory */
//  37 -- one short





//      /* the declarations below are only applicable to links */
//      
// 24  .    2 pad3 bit (18),
//     1    2 pathname_size fixed bin (17),                         /* number of characters in pathname */
//      
// 25 42    2 pathname char (168 refer (pathname_size))) unaligned, /* pathname of link */
//      
// 67  1    2 checksum bit (36),                                    /* checksum from uid */
//      
// 68  1    2 owner bit (36);                                       /* uid of containing directory */
//      

// .dsk files 
// data is in packed 72 format
// word72 <=> uchar [9]


#define SECTOR64_SZ_IN_W36 64
#define SECTOR64_SZ_IN_BYTES ((36 * SECTOR64_SZ_IN_W36) / 8)
typedef uint8_t sector64 [SECTOR64_SZ_IN_BYTES];

#define SECTOR_SZ_IN_W36 512
#define SECTOR_SZ_IN_BYTES ((36 * SECTOR_SZ_IN_W36) / 8)
//#define RECORD_SZ_IN_BYTES (sect_per_rec * SECTOR_SZ_IN_BYTES)
#define RECORD_SZ_IN_BYTES (2 * SECTOR_SZ_IN_BYTES)

//typedef uint8_t sector [SECTOR_SZ_IN_BYTES];
typedef uint8_t record [RECORD_SZ_IN_BYTES];


// Track blocks used
static bool filemap_used [N_SVS] [01000000];
static int  filemap_vtoce [N_SVS] [01000000];
static int  filemap_fmi [N_SVS] [01000000];

// Layout of data as read from simh tape format
//
//   bits: buffer of bits from a simh tape. The data is
//   packed as 2 36 bit words in 9 eight bit bytes (2 * 36 == 7 * 9)
//   The of the bytes in bits is
//      byte     value
//       0       most significant byte in word 0
//       1       2nd msb in word 0
//       2       3rd msb in word 0
//       3       4th msb in word 0
//       4       upper half is 4 least significant bits in word 0
//               lower half is 4 most significant bit in word 1
//       5       5th to 13th most signicant bits in word 1
//       6       ...
//       7       ...
//       8       least significant byte in word 1
//

// Multics humor: this is idiotic


// Data conversion routines
//
//  'bits' is the packed bit stream read from the simh tape
//    it is assumed to start at an even word36 address
//
//   extr36
//     extract the word36 at woffset
//

word36 extr36 (uint8_t * bits, uint woffset)
  {
    uint isOdd = woffset % 2;
    uint dwoffset = woffset / 2;
    uint8_t * p = bits + dwoffset * 9;

    uint64_t w;
    if (isOdd)
      {
        w  = ((uint64_t) (p [4] & 0xf)) << 32;
        w |=  (uint64_t) (p [5]) << 24;
        w |=  (uint64_t) (p [6]) << 16;
        w |=  (uint64_t) (p [7]) << 8;
        w |=  (uint64_t) (p [8]);
      }
    else
      {
        w  =  (uint64_t) (p [0]) << 28;
        w |=  (uint64_t) (p [1]) << 20;
        w |=  (uint64_t) (p [2]) << 12;
        w |=  (uint64_t) (p [3]) << 4;
        w |= ((uint64_t) (p [4]) >> 4) & 0xf;
      }
    // mask shouldn't be neccessary but is robust
    return (word36) (w & 0777777777777ULL);
  }

void put36 (word36 val, uint8_t * bits, uint woffset)
  {
    uint isOdd = woffset % 2;
    uint dwoffset = woffset / 2;
    uint8_t * p = bits + dwoffset * 9;

    if (isOdd)
      {
        p [4] &=               0xf0;
        p [4] |= (val >> 32) & 0x0f;
        p [5]  = (val >> 24) & 0xff;
        p [6]  = (val >> 16) & 0xff;
        p [7]  = (val >>  8) & 0xff;
        p [8]  = (val >>  0) & 0xff;
        //w  = ((uint64_t) (p [4] & 0xf)) << 32;
        //w |=  (uint64_t) (p [5]) << 24;
        //w |=  (uint64_t) (p [6]) << 16;
        //w |=  (uint64_t) (p [7]) << 8;
        //w |=  (uint64_t) (p [8]);
      }
    else
      {
        p [0]  = (val >> 28) & 0xff;
        p [1]  = (val >> 20) & 0xff;
        p [2]  = (val >> 12) & 0xff;
        p [3]  = (val >>  4) & 0xff;
        p [4] &=               0x0f;
        p [4] |= (val <<  4) & 0xf0;
        //w  =  (uint64_t) (p [0]) << 28;
        //w |=  (uint64_t) (p [1]) << 20;
        //w |=  (uint64_t) (p [2]) << 12;
        //w |=  (uint64_t) (p [3]) << 4;
        //w |= ((uint64_t) (p [4]) >> 4) & 0xf;
      }
    // mask shouldn't be neccessary but is robust
  }

//
//   extr9
//     extract the word9 at coffset
//
//   | 012345678 | 012345678 |012345678 | 012345678 | 012345678 | 012345678 | 012345678 | 012345678 |
//     0       1          2         3          4          5          6          7          8
//     012345670   123456701  234567012   345670123   456701234   567012345   670123456   701234567  
//

word9 extr9 (uint8_t * bits, uint coffset)
  {
    uint charNum = coffset % 8;
    uint dwoffset = coffset / 8;
    uint8_t * p = bits + dwoffset * 9;

    word9 w;
    switch (charNum)
      {
        case 0:
          w = ((((word9) p [0]) << 1) & 0776) | ((((word9) p [1]) >> 7) & 0001);
          break;
        case 1:
          w = ((((word9) p [1]) << 2) & 0774) | ((((word9) p [2]) >> 6) & 0003);
          break;
        case 2:
          w = ((((word9) p [2]) << 3) & 0770) | ((((word9) p [3]) >> 5) & 0007);
          break;
        case 3:
          w = ((((word9) p [3]) << 4) & 0760) | ((((word9) p [4]) >> 4) & 0017);
          break;
        case 4:
          w = ((((word9) p [4]) << 5) & 0740) | ((((word9) p [5]) >> 3) & 0037);
          break;
        case 5:
          w = ((((word9) p [5]) << 6) & 0700) | ((((word9) p [6]) >> 2) & 0077);
          break;
        case 6:
          w = ((((word9) p [6]) << 7) & 0600) | ((((word9) p [7]) >> 1) & 0177);
          break;
        case 7:
          w = ((((word9) p [7]) << 8) & 0400) | ((((word9) p [8]) >> 0) & 0377);
          break;
      }
    // mask shouldn't be neccessary but is robust
    return w & 0777U;
  }

//
//   extr18
//     extract the word18 at coffset
//
//   |           11111111 |           11111111 |           11111111 |           11111111 |
//   | 012345678901234567 | 012345678901234567 | 012345678901234567 | 012345678901234567 |
//
//     0       1       2          3       4          5       6          7       8
//     012345670123456701   234567012345670123   456701234567012345   670123456701234567  
//
//     000000001111111122   222222333333334444   444455555555666666   667777777788888888
//
//       0  0  0  0  0  0     0  0  0  0  0  0     0  0  0  0  0  0     0  0  0  0  0  0
//       7  7  6  0  0  0     7  7  0  0  0  0     7  4  0  0  0  0     6  0  0  0  0  0
//       0  0  1  7  7  4     0  0  7  7  6  0     0  3  7  7  0  0     1  7  7  4  0  0
//       0  0  0  0  0  3     0  0  0  0  1  7     0  0  0  0  7  7     0  0  0  3  7  7

word18 extr18 (uint8_t * bits, uint boffset)
  {
    uint byteNum = boffset % 4;
    uint dwoffset = boffset / 4;
    uint8_t * p = bits + dwoffset * 18;

    word18 w;
    switch (byteNum)
      {
        case 0:
          w = ((((word18) p [0]) << 10) & 0776000) | ((((word18) p [1]) << 2) & 0001774) | ((((word18) p [2]) >> 6) & 0000003);
          break;
        case 1:
          w = ((((word18) p [2]) << 12) & 0770000) | ((((word18) p [3]) << 4) & 0007760) | ((((word18) p [4]) >> 4) & 0000017);
          break;
        case 2:
          w = ((((word18) p [4]) << 14) & 0740000) | ((((word18) p [5]) << 6) & 0037700) | ((((word18) p [6]) >> 2) & 0000077);
          break;
        case 3:
          w = ((((word18) p [6]) << 16) & 0600000) | ((((word18) p [7]) << 8) & 0177400) | ((((word18) p [8]) >> 0) & 0000377);
          break;
      }
    // mask shouldn't be neccessary but is robust
    return w & 0777777U;
  }

//
//  getbit
//     Get a single bit. offset can be bigger when word size
//

uint8_t getbit (void * bits, int offset)
  {
    int offsetInWord = offset % 36;
    int revOffsetInWord = 35 - offsetInWord;
    int offsetToStartOfWord = offset - offsetInWord;
    int revOffset = offsetToStartOfWord + revOffsetInWord;

    uint8_t * p = (uint8_t *) bits;
    unsigned int byte_offset = revOffset / 8;
    unsigned int bit_offset = revOffset % 8;
    // flip the byte back
    bit_offset = 7 - bit_offset;

    uint8_t byte = p [byte_offset];
    byte >>= bit_offset;
    byte &= 1;
    //printf ("offset %d, byte_offset %d, bit_offset %d, byte %x, bit %x\n", offset, byte_offset, bit_offset, p [byte_offset], byte);
    return byte;
  }

//
// extr
//    Get a string of bits (up to 64)
//

uint64_t extr (void * bits, int offset, int nbits)
  {
    uint64_t n = 0;
    int i;
    for (i = nbits - 1; i >= 0; i --)
      {
        n <<= 1;
        n |= getbit (bits, i + offset);
        //printf ("012lo\n", n);
      }
    return n;
  }


static int r2s (int rec, int sv)
  {
    int usable = (sect_per_cyl / sect_per_rec) * sect_per_rec;
    int unusable = sect_per_cyl - usable;
    int sect = rec * sect_per_rec;
    sect = sect + (sect / usable) * unusable; 

    int sect_offset = sect % sect_per_cyl;
    sect = (sect - sect_offset) * number_of_sv + sv * sect_per_cyl +
            sect_offset;
//printf ("r2s %d -> %d\n", rec, sect);
    return sect;
  }

static char * str (word36 w)
  {
    static char buf [5];
    buf [0] = (w >> 27) & 0377;
    buf [1] = (w >> 18) & 0377;
    buf [2] = (w >>  9) & 0377;
    buf [3] = (w >>  0) & 0377;
    buf [4] = 0;
    for (int i = 0; i < 4; i ++)
      if (! isprint (buf [i]))
        buf [i] = '?';
    return buf;
  }

static struct
  {
    int rec;
    int sv;
    record data;
  } cache = { -1, -1, { 1024 * 0 } };

static void readSector64 (int fd, int sect, uint8_t * data)
  {
    off_t n = lseek (fd, sect * SECTOR64_SZ_IN_BYTES, SEEK_SET);
    if (n == (off_t) -1)
      {
        fprintf (stderr, "seek off of end of data\n");
        exit (1);
      }
    ssize_t r = read (fd, data, SECTOR64_SZ_IN_BYTES);
    if (r != SECTOR64_SZ_IN_BYTES)
      {
        fprintf (stderr, "read() returned wrong size\n");
        exit (1);
      }
//printf ("data %012lo %012lo %012lo %012lo %012lo %012lo\n", data[0],data[1],data[2],data[3],data[4],data[5]);
  }

static int last_sect;

static void readRecord (int fd, int rec, int sv, record * data)
  {
    if (cache . rec == rec && cache . sv == sv)
      {
        memcpy (data, & cache . data, sizeof (record));
        return;
      }

    int sect = r2s (rec, sv);
    last_sect = sect;
    off_t n = lseek (fd, sect * SECTOR_SZ_IN_BYTES, SEEK_SET);
    if (n == (off_t) -1)
      {
        fprintf (stderr, "seek off of end of data\n");
        exit (1);
      }
    ssize_t r = read (fd, & cache . data, sizeof (record));
    if (r != sizeof (record))
      {
        fprintf (stderr, "read() returned wrong size\n");
        exit (1);
      }
    cache . rec = rec;
    cache . sv = sv;
    memcpy (data, & cache . data, sizeof (record));
  }

#define MASK36 0777777777777
#define MASK24 0000077777777
#define MASK18 0000000777777

static word36 vtoc_origin = 8;
static word36 vtoc_header = 4;
typedef word36 VTOCE  [512];

static int vtoce_recnum;
static int vtoce_offset;

static void readVTOCE (struct m_state * m_data, int entNo, int sv, VTOCE * data)
  {
    if (m_data->is512)
      {
        // 2 VOTCE / record; VTOCE is at 8.
        int recOff = entNo / 2;
        int recNum = recOff + 8;
        record vtocepair;
        memset (& vtocepair, 0, sizeof (record));
        readRecord (m_data->fd, recNum, sv, & vtocepair);
        int offset = (entNo & 1) ? 512 : 0;
        for (int i = 0; i < 512; i ++)
          (* data) [i] = extr36 (vtocepair, offset + i);
        vtoce_recnum = recNum;
        vtoce_offset = offset;
      }
    else
      {
#if 1
        // 5 VOTCE / record; VTOCE is at 8.
        int recOff = entNo / 5;
        int recNum = recOff + 8;
        record vtoceset;
        memset (& vtoceset, 0, sizeof (record));
        readRecord (m_data->fd, recNum, sv, & vtoceset);
        int offset = (entNo % 5) * 192;
        for (int i = 0; i < 192; i ++)
          (* data) [i] = extr36 (vtoceset, offset + i);
        vtoce_recnum = recNum;
        vtoce_offset = offset;
#else
        // VOTCE is three 64 word sectors
        // VTOC starts at record 8; convert to sector #.
        //  Each record is 1024 words, or 16 sectors
        int vtoc_start_sector = 8 * 16;
        // Each entry is 3 sectors
        int vtoc_entry_start = vtoc_start_sector + entNo * 3;
        uint8_t sector [SECTOR64_SZ_IN_BYTES];
        int n = 0;
        for (int s = 0; s < 3; s ++)
          {
            readSector64 (m_data->fd, vtoc_entry_start + s, sector);
            //for (int i = 0; i < 64; i ++)
              //printf ("%d %3d %03o\n", s, i, vtoce [i]);
              //printf ("%d %3d %012lo\n", s, i, extr36 (vtoce, i));
            for (int i = 0; i < 64; i ++)
              (* data) [n++] = extr36 (sector, i);
          }
        vtoce_recnum = vtoc_entry_start;
        vtoce_offset = 0;
#endif
      }
  }

static void readFileDataRecord (struct m_state * m_data, int ind, uint frecno, record * data)
  {
    uint recno = m_data -> vtoc [ind] . filemap [frecno];
    if (recno & 0400000)
      {
        fprintf (stderr, "read of non-allocated data record\n");
        memset (data, 0, sizeof (record));
        return;
      }

    readRecord (m_data -> fd, recno, m_data -> vtoc [ind] . sv, data);
  }

static word36 readFileDataWord36 (struct m_state * m_data, int ind, uint wordno)
  {
    // 1204 words/record.
    uint frecno = wordno / 1024;
    uint offset = wordno % 1024;
    record rdata;
    readFileDataRecord (m_data, ind, frecno, & rdata);
    return extr36 (rdata, offset);
  }

#if 0
static word72 readFileDataWord72 (struct m_state * m_data, int ind, uint wordno)
  {
    // 1204 words/record.
    uint frecno = wordno / 1024;
    uint offset = wordno % 1024;
    record rdata;
    readFileDataRecord (m_data, ind, frecno, & rdata);
    word36 even = extr36 (rdata, offset);
    word36 odd = extr36 (rdata, offset + 1);
    return ((word72) even << 36) | odd;
  }
#endif

static void unfixit (char * s)
  {
    size_t l = strlen (s);
    for (size_t i = 0; i < l; i ++)
      if (s [i] == '>')
        s [i] = '/';
  }

static void fixit (char * s)
  {
    size_t l = strlen (s);
    for (size_t i = 0; i < l; i ++)
      if (s [i] == '/')
        s [i] = '>';
  }

// return index into uid table; -1 if no such file or directory
int mx_lookup_path (struct m_state * m_data , const char * path)
  {
    if (path [0] != '/')
      {
        //log_msg ("mx_lookup_path not at root (%s)\n", path);
        return -1;
      }
    char * s = strdup (path);
    fixit (s);
    for (int i = 0; i < m_data -> vtoc_cnt; i ++)
      {
//log_msg ("%s %s\n", s, m_data -> vtoc [i] . fq_name);
        if (strcmp (s, m_data -> vtoc [i] . fq_name) == 0)
          return i;
      }
    return -1;
  }

static int find_uid (struct m_state * m_data, word36 uid)
  {
    int vtoc_cnt = m_data -> vtoc_cnt;
    struct vtoc * vtoc = m_data -> vtoc;
    for (int i = 0; i < vtoc_cnt; i ++)
      if (vtoc [i] . uid == uid)
        return i;
//fprintf (stderr, "find_uid vtoc_cnt %d\n", vtoc_cnt);
#ifdef DEBUG
    for (int i = 0; i < vtoc_cnt; i ++)
      dprintf (stderr, "%5d %012lo\n", i, vtoc [i] . uid);
#endif

    return -1;
  }

static void processDirectory (struct m_state * m_data, int ind)
  {
printf ("  %s\n", m_data->vtoc[ind].fq_name);
    struct vtoc * vtocp = m_data -> vtoc + ind;
    word36 type_size = readFileDataWord36 (m_data, ind, 1);
    if (type_size != 0000003000100lu)
      {
        printf ("error in dir header type/size for ind %d %012lo\n", ind, type_size);
        fprintf (stderr, "error in dir header type/size for ind %d %012lo\n", ind, type_size);
        //return;
      }
    word36 vtocx_vers = readFileDataWord36 (m_data, ind, 13);
    if ((vtocx_vers & MASK18) != 2)
      {
        printf ("error in dir header version for ind %d %012lo\n", ind, vtocx_vers);
        fprintf (stderr, "error in dir header version for ind %d\n", ind);
        return;
      }

    word36 seg_dir_cnt = readFileDataWord36 (m_data, ind, 18);
    vtocp -> seg_cnt = (seg_dir_cnt >> 18) & MASK18;
    vtocp -> dir_cnt = seg_dir_cnt & MASK18;
    word36 lcnt_acle  = readFileDataWord36 (m_data, ind, 19);
    vtocp -> lnk_cnt = (lcnt_acle >> 18) & MASK18;

    vtocp -> ent_cnt = vtocp -> seg_cnt + vtocp -> dir_cnt + vtocp -> lnk_cnt;
    vtocp -> entries = calloc (sizeof (struct entry), vtocp -> ent_cnt);
    if (vtocp -> entries == NULL)
      {
        perror ("entries alloc");
        abort ();
      }

    word36 entryfrpw = readFileDataWord36 (m_data, ind, 14);
    int entryfrp = (entryfrpw >> 18) & MASK18;
printf ("     seg_cnt %d dir_cnt %d lnk_cnt %d ent_cnt %d entryfrp %d\n", vtocp -> seg_cnt, vtocp -> dir_cnt, vtocp -> lnk_cnt, vtocp -> ent_cnt, entryfrp);
    //word36 entrybrp = readFileDataWord36 (m_data, ind, 15);
    //entrybrp = (entrybrp >> 18) & MASK18;

    int entry_cnt = 0;
    for (int entryp = entryfrp; entryp; )
      {
        word36 rp = readFileDataWord36 (m_data, ind, entryp);
        word18 efrp = (rp >> 18) & MASK18;
        //word18 ebrp = rp & MASK18;

        word36 type_size = readFileDataWord36 (m_data, ind, entryp + 1);
        word18 type = (type_size >> 18) & MASK18;
        if (type == 0)
          {
            goto next;
          }

        word36 uid = readFileDataWord36 (m_data, ind, entryp + 2);
        
        if (entry_cnt >= vtocp -> ent_cnt)
          {
            fprintf (stderr, "entries overflow\n");
            abort ();
          }


        char name [33 + 100];
        name [0] = 0;
        for (int j = 0; j < 8; j ++)
//  entry include file says that the name starts at offset 8, but data dumps indicate offset 12
          strcat (name, str (readFileDataWord36 (m_data, ind, entryp + 8 + 4 + j)));
        for (int j = strlen (name) - 1; j >= 0; j --)
          if (name [j] == ' ')
            name [j] = 0;
          else
            break;

        if (find_uid (m_data, uid) < 0)
          {
            printf ("    virtual file entryp: %d type: %d uid: %012lo name: '%s'\n", entryp, type, uid, name);
          }
        else
          {
            printf ("    entryp: %d type: %d uid: %012lo name: '%s'\n", entryp, type, uid, name);
// 4 directory
// 5 link
// 7 segment
            if (type != 7 && type != 4 && type !=5)
              printf ("    %d %s\n", type, name);
            vtocp -> entries [entry_cnt] . name = strdup (name);
            vtocp -> entries [entry_cnt] . uid = uid;
            vtocp -> entries [entry_cnt] . type = type;
            word36 bc = readFileDataWord36 (m_data, ind, entryp + 32);
            vtocp -> entries [entry_cnt] . bitcnt = bc & MASK24;

            if (type == 5) // link
              {
                word18 pathname_size = readFileDataWord36 (m_data, ind, entryp + 24) & MASK18;
                if (pathname_size > 168)
                  {
                    fprintf (stderr, "pathname_size %u truncated\n", pathname_size);
                    pathname_size = 168;
                  }
                char pathname [169];
                pathname [0] = 0;
                for (int j = 0; j < 42; j ++)
                  strcat (pathname, str (readFileDataWord36 (m_data, ind, entryp + 25 + j)));
                pathname [pathname_size] = 0;
                //printf ("[%s]\n", pathname);
                vtocp -> entries [entry_cnt] . link_target = strdup (pathname);
              }
            else
              {
                char path [8192];
                strcpy (path, vtocp -> fq_name);
                if (strcmp (path, ">") != 0)
                  strcat (path, ">");
                strcat (path, name);
                unfixit (path);
                vtocp -> entries [entry_cnt] . pri_ind = mx_lookup_path (m_data, path);
              }
          }
        entry_cnt ++;
next:;
        entryp = efrp;
      }
    if (entry_cnt != vtocp -> ent_cnt)
      printf ("entry_cnt %d ent_cnt %d\n", entry_cnt, vtocp -> ent_cnt);
  }

static void usage (void)
  {
    printf ("Usage: analyze device_type image n_svs\n");
    printf ("  device_type:  3381 501\n");
    exit (1);
  }

// return
//  0 ok
//  -1 Can't open disk image
//  -2 Not a Multics volume

static int n_svs;

static int mx_mount (struct m_state * m_data)
  {
    m_data -> fd = open (m_data -> dsknam, O_RDONLY);
    if (m_data -> fd < 0)
      usage ();
  
   memset (filemap_used, 0, (sizeof filemap_used));

// print pvids

    record r0;
    for (int sv = 0; sv < n_svs; sv ++)
      { 
        memset (& r0, 0, sizeof (record));
        readRecord (m_data->fd, 0, sv, & r0);

        //int offset = label_perm_os;
        word36 w, w2;

        printf ("\nsv %c\n", 'a' + sv);

        printf ("  Permanent label:\n");

        printf ("    raw:\n");
        for (uint i = 0; i < 64; i++)
          {
            w = extr36 (r0, label_perm_os + i);
            printf ("    %2d %012"PRIo64"\n", i, w);
          }

        printf ("    label: ");
        for (uint i = 0; i < 8; i++)
          {
            w = extr36 (r0, label_perm_os +  0 + i);
            printf ("%s", str (w));
          }
        printf ("\n");

        w = extr36 (r0, label_perm_os +  8);
        printf ("    version: %"PRId64"\n", w);

        printf ("    Mfg serial number: ");
        for (uint i = 0; i < 8; i++)
          {
            w = extr36 (r0, label_perm_os +  9 + i);
            printf ("%s", str (w));
          }
        printf ("\n");

        printf ("    PV name: ");
        for (uint i = 0; i < 8; i++)
          {
            w = extr36 (r0, label_perm_os +  17 + i);
            printf ("%s", str (w));
          }
        printf ("\n");

        printf ("    LV name: ");
        for (uint i = 0; i < 8; i++)
          {
            w = extr36 (r0, label_perm_os +  25 + i);
            printf ("%s", str (w));
          }
        printf ("\n");

        w = extr36 (r0, label_perm_os +  33);
        printf ("    PV id: %012"PRIo64"\n", w);

        w = extr36 (r0, label_perm_os +  34);
        printf ("    LV id: %012"PRIo64"\n", w);

        w = extr36 (r0, label_perm_os +  35);
        printf ("    root PV id: %012"PRIo64"\n", w);

        w = extr36 (r0, label_perm_os +  36);
        w2 = extr36 (r0, label_perm_os +  37);
        printf ("    time registered: %012"PRIo64" %012"PRIo64"\n", w, w2);

        w = extr36 (r0, label_perm_os +  38);
        printf ("    number of PVs in LV: %"PRId64"\n", w);

        w = extr36 (r0, label_perm_os +  39);
        printf ("    volume size in records: %"PRId64"\n", w);

        w = extr36 (r0, label_perm_os +  40);
        printf ("    vtoc + fixed area  size in records: %"PRId64"\n", w);

        w = extr36 (r0, label_perm_os +  41);
        printf ("    private: %d\n", (w & 0200000000000) ? 1 : 0);
        printf ("    iconsistent_dbm (crash w/o eds): %d\n", (w & 0100000000000) ? 1 : 0);

        w = extr36 (r0, label_perm_os +  42);
        w2 = extr36 (r0, label_perm_os +  43);
        printf ("    max access class: %012"PRIo64" %012"PRIo64"\n", w, w2);

        // 45, 46 password; not used

        w = extr36 (r0, label_perm_os +  47);
        printf ("    number of sub-volumes: %"PRId64"\n", w);

        w = extr36 (r0, label_perm_os +  48);
        printf ("    this sub-volume: %"PRId64"\n", w);

        w = extr36 (r0, label_perm_os +  49);
        char ch = (w >> 27) & 0xff;
        printf ("    sub-volume name: %c (%hho)\n", isgraph(ch) ? ch : '?', ch);

// root record

        printf ("  root record\n");

        printf ("    raw:\n");
        for (uint i = 0; i < 64; i++)
          {
            w = extr36 (r0, label_root_os + i);
            printf ("    %2d %012"PRIo64"\n", i, w);
          }

        w = extr36 (r0, label_root_os +  0);
        printf ("    root_here: %c\n", (w & 0400000000000) ? 'y' : 'n');

        w = extr36 (r0, label_root_os +  1);
        printf ("    root vtocx: %"PRId64"\n", w & 0377777777777);

        w = extr36 (r0, label_root_os +  2);
        printf ("    shutdown state: %"PRId64"\n", w);

        w = extr36 (r0, label_root_os +  3);
        printf ("    unused: %012"PRIo64"\n", w);

        w = extr36 (r0, label_root_os +  4);
        printf ("    disk table vtocx: %"PRId64"\n", w & 0377777777777);

        w = extr36 (r0, label_root_os +  5);
        printf ("    disk table uid: %"PRId64"\n", w);

        w = extr36 (r0, label_root_os +  6);
        printf ("    esd state: %"PRId64"\n", w);

        w = extr36 (r0, label_root_os +  12);
        printf ("    vtoc origin record: %"PRId64"\n", w);


        printf ("  partition record\n");

        printf ("    raw:\n");
        for (uint i = 0; i < 64; i++)
          {
            w = extr36 (r0, label_part_os + i);
            printf ("    %2d %012"PRIo64"\n", i, w);
          }

        w = extr36 (r0, label_part_os +   3);
        printf ("    nparts: %"PRId64"\n", w);

        int nparts = w;
        if (nparts > 47) nparts = 47;
        for (int i = 0; i < nparts; i ++)
          {
            printf ("      partition %d\n", i);
            int pos = i * 4 + 4;
            w = extr36 (r0, label_part_os + pos + 0);
            printf ("        parts: %s\n", str (w));
            w = extr36 (r0, label_part_os + pos + 1);
            printf ("        frec: %"PRId64"\n", w);
            w = extr36 (r0, label_part_os + pos + 2);
            printf ("        nrec: %"PRId64"\n", w);
            w = extr36 (r0, label_part_os + pos + 3);
            printf ("        pad5: %"PRId64"\n", w);
          }
      }



// Get the disk label; verify that it is a Multics volume

    memset (& r0, 0, sizeof (record));
    readRecord (m_data->fd, 0, 0, & r0);
#if 0
    for (int i = 0; i < 1024; i ++)
      {
        word36 w = extr36 (r0, i);
        printf ("012lo %s\n", w, str (w));
      }
#endif
// Identifer is Multics char (32) init ("Multics Storage System Volume")
//  
// 115165154164 Mult.
// 151143163040 ics .
// 123164157162 Stor.
// 141147145040 age .
// 123171163164 Syst.
// 145155040126 em V.
// 157154165155 olum.
// 145040040040 e   .

    word36 mlabel [8] = 
      {
        0115165154164lu,
        0151143163040lu,
        0123164157162lu,
        0141147145040lu,
        0123171163164lu,
        0145155040126lu,
        0157154165155lu,
        0145040040040lu
     };

    for (uint i = 0; i < 8; i ++)
      if (extr36 (r0, label_perm_os + i) != mlabel [i])
      {
        printf ("Not a Multics volume (%012"PRIo64" != %012"PRIo64")\n", extr36 (r0, label_perm_os + i), mlabel [i]);
        return -2;
      }

// Unmounted properly?
// AN61, pg 14-2 "THis,. if an attempt is made to accept a physical volume
// for which the value of label.time_map_updated and the value of 
// lable.time_unmounted are not equal, then the volume was improperly
// shutdown.

    word36 time_map_upd = extr36 (r0, label_time_map_updated_os);
    word36 time_unmounted = extr36 (r0, label_time_unmounted_os);

    //printf ("Time map updated %012"PRIo64"\n", time_map_upd);
    //printf ("Time unmounted   %012"PRIo64"\n", time_unmounted);

    if (time_map_upd != time_unmounted)
      printf ("WARNING: Not dismounted properly\n");

    //printf ("pack PVID %012"PRIo64"\n", extr36 (r0, label_perm_os + 33));
    //printf ("pack LVID %012"PRIo64"\n", extr36 (r0, label_perm_os + 34));

// What is the root VTOC index?

    word36 root_vtoc = extr36 (r0, label_root_os + 0);
    if (root_vtoc & 0400000000000lu)
      {
        root_vtoc &= 0377777777777lu;
        printf ("Root is on this pack; VOTC index of %012"PRIo64" (%"PRIu64")\n", root_vtoc, root_vtoc);
      }
    else
      {
        printf ("Root is not on this pack\n");
      }

    word36 vtoc_size = extr36 (r0, label_perm_os + 39);
    printf ("vtoc_size %"PRIu64"\n", vtoc_size);

// Lack of understanding. According to ddl the VTOC region is first record 8 (10o)  size 7487 (16477o)  
#if 0
// Where be the VTOC?

    word36 vtoc_origin = extr36 (r0, label_vtoc_origin_record_os);
    printf ("VTOC origin %012"PRIo64"\n", vtoc_origin);
    for (int i = label_root_os; i < label_root_os + 20; i ++)
      {
        word36 w = extr36 (r0, i);
        printf ("012lo %s\n", w, str (w));
      }
#else
    vtoc_origin = 8;
    vtoc_header = 4;
#endif

    int total_vtoc_no = 0;
    for (int sv = 0; sv < n_svs; sv ++)
      {
        printf ("\n\n Analyzing %c vtoc header\n\n", 'a' + sv);
        record vtoch;
        memset (& vtoch, 0, sizeof (record));
        readRecord (m_data->fd, vtoc_header, sv, & vtoch);
        word36 n_vtoces = extr36 (vtoch, vtoc_header_n_vtoce_os);
        printf ("n_vtoces %"PRIu64"\n", n_vtoces);
        //word36 n_free_vtoces = extr36 (vtoch, vtoc_header_n_free_vtoce);
        word36 vtoc_last_recno = extr36 (vtoch, vtoc_header_vtoc_last_recno);
    
        word36 vtoc_sz_recs = vtoc_last_recno + 1 - vtoc_origin;
        m_data->vtoc_no [sv] = (int) (vtoc_sz_recs * 2);
        total_vtoc_no += m_data->vtoc_no [sv];
        printf ("vtoc_no %d\n", m_data->vtoc_no[sv]);
      }

    m_data->vtoc = calloc (sizeof (struct vtoc), total_vtoc_no);
    if (m_data->vtoc == NULL)
      {
        perror ("vtoc alloc");
        abort ();
      }

// Build uid, attr and name table

    m_data->vtoc_cnt = 0;
    for (int sv = 0; sv < n_svs; sv ++)
      {
        printf ("\n\n Reading %c vtocs\n\n", 'a' + sv);
       for (int i = 0; i < m_data->vtoc_no [sv]; i ++)
          {
            VTOCE vtoce;
            readVTOCE (m_data, i, sv, & vtoce);
            word36 uid = vtoce [1];
printf ("sv %d vtocno %d uid %012lo\n", sv, i, uid);
            if (! uid)
              {
#if 0
                char name [33 + 100];
                name [0] = 0;
                for (int j = 0; j < 8; j ++)
                   strcat (name, str (vtoce [vtoce_primary_name_os + j]));
                for (int j = strlen (name) - 1; j >= 0; j --)
                   if (name [j] == ' ')
                     name [j] = 0;
                   else
                     break;
                printf ("   %s\n", name);
#endif
                //printf ("   %5d [%o:%d] <emtpy>\n", i, vtoce_recnum, vtoce_offset);
                continue;
              }
            m_data->vtoc [m_data->vtoc_cnt] . uid = uid;
            m_data->vtoc [m_data->vtoc_cnt] . attr = vtoce [5];
            m_data->vtoc [m_data->vtoc_cnt] . dtu = vtoce [3];
            m_data->vtoc [m_data->vtoc_cnt] . dtm = vtoce [4];
            m_data->vtoc [m_data->vtoc_cnt] . time_created = vtoce [184];
            m_data->vtoc [m_data->vtoc_cnt] . sv = sv;
            m_data->vtoc [m_data->vtoc_cnt] . vtoce= i;
            for (uint fmi = 0; fmi < 128; fmi ++)
              {
                for (uint fmix = 0; fmix < 2; fmix ++)
                  {
                    int32_t recno;
                    if (fmix == 0)
                      recno  = (vtoce [vtoce_fm_os + fmi] >> 18) & MASK18;
                    else
                      recno  = (vtoce [vtoce_fm_os + fmi]      ) & MASK18;
                    if (! (recno & 0400000))
                      {
                        if (filemap_used[sv][recno])
                          {
                            printf ("filemap duplicate sv %d vtoce %d fmi %d recno %d\n", sv, i, fmi * 2 + fmix, recno);
                            fprintf (stderr, "filemap duplicate sv %d vtoce %d fmi %d recno %d\n", sv, i, fmi * 2 + fmix, recno);
                          }
                        else
                          {
                            filemap_used[sv][recno] = true;
                            filemap_vtoce[sv][recno] = i;
                            filemap_fmi[sv][recno] = fmi * 2 + fmix;
                          }
                      }
                    m_data->vtoc [m_data->vtoc_cnt] . filemap [fmi * 2 + fmix] = recno;
                  }
              }

            if (uid == 0777777777777lu) // root
              {
                m_data->vtoc [m_data->vtoc_cnt] . name = strdup (">");
              }
            else
              {
                char name [33 + 100];
                name [0] = 0;
                for (int j = 0; j < 8; j ++)
                   strcat (name, str (vtoce [vtoce_primary_name_os + j]));
                for (int j = strlen (name) - 1; j >= 0; j --)
                   if (name [j] == ' ')
                     name [j] = 0;
                   else
                     break;
                m_data->vtoc [m_data->vtoc_cnt] .name = strdup (name);
              }
            printf ("   %5d. %6o [%5o:%4o][%6o] %012lo %s\n", i, i, vtoce_recnum, vtoce_offset, last_sect, uid, m_data->vtoc[m_data->vtoc_cnt].name);
            for (uint i = 0; i < 256; i += 8)
              {
                printf ("       %5d: ", i);
                for (uint j = 0; j < 8; j ++)
                  printf ("%8d", m_data->vtoc[m_data->vtoc_cnt].filemap [i + j]);
                printf ("\n");
              }
            m_data->vtoc_cnt ++;
          }
      }


// Build dir_name & fq_name table

    printf ("\n\n Building names\n\n");
    for (int i = 0; i < m_data->vtoc_cnt; i ++)
      {
        char fq_name [4096];
        fq_name [0] = 0;

        VTOCE vtoce;
        readVTOCE (m_data, m_data->vtoc [i] . vtoce, m_data->vtoc [i] . sv, & vtoce);
        for (int j = 0; j < 16; j ++)
          {
            word36 path_uid = vtoce [160 + j];
            if (! path_uid)
              break;
            int k;
            for (k = 0; k < m_data->vtoc_cnt; k ++)
              {
                if (m_data->vtoc [k] . uid == path_uid)
                  {
                    strcat (fq_name, m_data->vtoc [k] . name);
                    break;
                  }
              }
            if (k >= m_data->vtoc_cnt)
              {
                 char buf [13];
                 sprintf (buf, "%012"PRIo64"", path_uid);
                 strcat (fq_name, buf);
              }
            if (j)
              strcat (fq_name, ">");
          }

        m_data->vtoc [i] . dir_name = strdup (fq_name);

        char name [33];
        name [0] = 0;
        for (int j = 0; j < 8; j ++)
          strcat (name, str (vtoce [vtoce_primary_name_os + j]));
        for (int j = strlen (name) - 1; j >= 0; j --)
          if (name [j] == ' ')
            name [j] = 0;
          else
            break;
        strcat (fq_name, name);
        m_data->vtoc [i] . fq_name = strdup (fq_name);
        printf ("   %5d %s\n", i, fq_name);
      }

// Build directory entries

    printf ("\n\n Build directory entries\n\n");

    for (int i = 0; i < m_data->vtoc_cnt; i ++)
      {
        if (m_data->vtoc [i] . attr & 0400000)
          processDirectory (m_data, i);
      }

    return 0;
  }



#if 0
int mx_read (char * buf, size_t size, off_t offset, struct entry * entryp)
  {
    struct m_state * m_data = M_DATA;

    uint byte_cnt = (entryp -> bitcnt + 7) / 8;
    if (offset > byte_cnt)
      return 0;

    size_t end = offset + size;
    if (end > byte_cnt)
      {
        size = byte_cnt - offset;
      }
    int writ = 0;
    while (size)
      {
        off_t recno = offset / RECORD_SZ_IN_BYTES;
        off_t recos = offset % RECORD_SZ_IN_BYTES;
        record rdata;
        readFileDataRecord (m_data, entryp -> pri_ind, recno, & rdata);
        size_t residue = RECORD_SZ_IN_BYTES - recos;
        uint mv;
        if (residue < size)
          mv = residue;
        else
          mv = size;
        memcpy (buf, rdata + recos, mv);
        buf += mv;
        size -= mv;
        offset += mv;
        writ += mv;
      }
    return writ;
  }
#endif

// analyze device_type img #_svs

int main (int argc, char * argv [])
  {
    struct m_state m_data;
    if (argc != 4)
      usage ();
    if (strcmp ("3381", argv[1]) == 0)
      {
        m_data.is512 = true;
        number_of_sv = 3;
        //sect_size = 512;
        sect_per_cyl = 255;
        sect_per_rec = 2;
      }
    else if (strcmp ("d501", argv[1]) == 0)
      {
        m_data.is512 = false;
        number_of_sv = 1;
        //sect_size = 64;
        sect_per_cyl = 1280;
        //sect_per_rec = 16;
        // Lie...
        sect_per_rec = 2;
      }
    else
      usage ();
    n_svs = atoi (argv[3]);
    if (n_svs < 1 || n_svs > N_SVS)
      usage ();
    m_data.dsknam = argv[2];
    mx_mount (&m_data);
  }
