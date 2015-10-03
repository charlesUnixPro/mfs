#include "mfs.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include "mfslib.h"


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
// 36  1      2 time_registered fixed bin (71),                       /* time imported to system */
// 37  1      2 n_pv_in_lv fixed bin,                                 /* # phys volumes in logical */
// 38  1      2 vol_size fixed bin,                                   /* total size of volume, in records */
// 39  1      2 vtoc_size fixed bin,                                  /* number of recs in fixed area + vtoc */
// 40  .      2 not_used bit (1) unal,                                /* used to be multiple_class */
//     .      2 private bit (1) unal,                                 /* TRUE if was registered as private */
//     .      2 inconsistent_dbm bit (1) unal,                        /* TRUE if ESD-less crash */
//     1      2 flagpad bit (33) unal,
// 41  2      2 max_access_class bit (72),                            /* Maximum access class for stuff on volume */
// 43  2      2 min_access_class bit (72),                            /* Minimum access class for stuff on volume */
// 45  2      2 password bit (72),                                    /* not yet used */
// 47  2      2 number_of_sv fixed bin,                               /* if = 0 not a subvolume else the number of svs */
// 49  1      2 this_sv fixed bin,                                    /* what subvolume number it is */
// 50  1      2 sub_vol_name char (1),                                /* what subvolume name (a b c d) it is */
// 51 13      2 pad1 (13) fixed bin,
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
//  0  .      2 root,
//     .        3 here bit (1),                                       /* TRUE if the root is on this pack */
//     1        3 root_vtocx fixed bin (35),                          /* VTOC index of root, if it is here */
//  1  1        3 shutdown_state fixed bin,                           /* Status of hierarchy */
//  2  1        3 pad7 bit (1) aligned,
//  3  1        3 disk_table_vtocx fixed bin,                         /* VTOC index of disk table on RPV */
//  4  1        3 disk_table_uid bit (36) aligned,                    /* UID of disk table */
//  5  1        3 esd_state fixed bin,                                /* State of esd */
//  6  1      2 volmap_record fixed bin,                              /* Begin record of volume map */
//  7  1      2 size_of_volmap fixed bin,                             /* Number of records in volume map */
//  8  1      2 vtoc_map_record fixed bin,                            /* Begin record of VTOC map */
//  9  1      2 size_of_vtoc_map fixed bin,                           /* Number of records in VTOC map */
// 10  1      2 volmap_unit_size fixed bin,                           /* Number of words per volume map section */
// 11  1      2 vtoc_origin_record fixed bin,                         /* Begin record of VTOC */
#define label_vtoc_origin_record_os (label_root_os + 11)
// 12  1      2 dumper_bit_map_record fixed bin,                      /* Begin record of dumper bit-map */
// 13  1      2 vol_trouble_count fixed bin,                          /* Count of inconsistencies found since salvage */
//    52      2 pad3 (52) fixed bin,
//     1      2 nparts fixed bin,                                     /* Number of special partitions on pack */
//   188      2 parts (47),
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
#define sect_per_cyl 255
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

//  fs_dev_types_sector.incl.pl1
//  
//  dcl  sect_per_cyl (9) fixed bin static options (constant) init /* table of # of sectors per cylinder on each device */
//      (0, 760, 760, 760, 589, 360, 1280, 255, 255);
#define sect_per_cyl 255
//  
//  dcl  sect_per_sv (9) fixed bin (24) static options (constant) init /* table of # of sectors per cylinder on each subvolume */
//       (0, 0, 0, 0, 0, 0, 0, 112710, 150450);
//  
//  dcl  sect_per_rec (9) fixed bin static options (constant) init
//   /* table of # of sectors per record on each device */
//   /* coresponding array in disk_pack.incl.pl1 called SECTORS_PER_RECORD */
//      (0, 16, 16, 16, 16, 16, 16, 2, 2);
#define sect_per_rec 2
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


// .dsk files 
// data is in packed 72 format
// word72 <=> uchar [9]


#define SECTOR_SZ_IN_W36 512
#define SECTOR_SZ_IN_BYTES ((36 * SECTOR_SZ_IN_W36) / 8)

typedef uint8_t sector [SECTOR_SZ_IN_BYTES];
typedef uint8_t record [sect_per_rec * SECTOR_SZ_IN_BYTES];


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
    //fprintf (stderr, "offset %d, byte_offset %d, bit_offset %d, byte %x, bit %x\n", offset, byte_offset, bit_offset, p [byte_offset], byte);
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
        //fprintf (stderr, "012lo\n", n);
      }
    return n;
  }


static int r2s (int rec)
  {
    int usable = (sect_per_cyl / sect_per_rec) * sect_per_rec;
    int unusable = sect_per_cyl - usable;
    int sect = rec * sect_per_rec;
    sect = sect + (sect / usable) * unusable; 
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

static void readRecord (int fd, int rec, record * data)
  {
    int sect = r2s (rec);
//fprintf (stderr, "rr rec %d sect %d\n", rec, sect);
    off_t n = lseek (fd, sect * SECTOR_SZ_IN_BYTES, SEEK_SET);
//fprintf (stderr, "rr rec %d sect %d offset %d %o\n", rec, sect, sect * 512, sect * 512);
    if (n == (off_t) -1)
      { fprintf (stderr, "2\n"); exit (1); }
    ssize_t r = read (fd, data, sizeof (record));
    if (r != sizeof (record))
      { fprintf (stderr, "3\n"); exit (1); }
  }

#define MASK36 0777777777777

static word36 vtoc_origin = 8;
static word36 vtoc_header = 4;
typedef word36 VTOCE  [512];

static void readVTOCE (int fd, int entNo, VTOCE * data)
  {
    // 2 VOTCE / record; VTOCE is at 8.
    int recOff = entNo / 2;
    int recNum = recOff + 8;
    record vtocepair;
    memset (& vtocepair, 0, sizeof (record));
    readRecord (fd, recNum, & vtocepair);
    int offset = (entNo & 1) ? 512 : 0;
    for (int i = 0; i < 512; i ++)
      (* data) [i] = extr36 (vtocepair, offset + i);
  }

// return
//  0 ok
//  -1 Can't open disk image
//  -2 Not a Multics volume

int mx_mount (struct m_state * m_data)
  {
    int fd = open (m_data -> dsknam, O_RDONLY);
    if (fd < 0)
      return -1;

// Get the disk label; verify that it is a Multics volume

    record r0;
    memset (& r0, 0, sizeof (record));
    readRecord (fd, 0, & r0);
#if 0
    for (int i = 0; i < 1024; i ++)
      {
        word36 w = extr36 (r0, i);
        fprintf (stderr, "012lo %s\n", w, str (w));
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
        fprintf (stderr, "Not a Multics volume (%012lo != %012lo)\n", extr36 (r0, label_perm_os + i), mlabel [i]);
        return -2;
      }

// Unmounted properly?
// AN61, pg 14-2 "THis,. if an attempt is made to accept a physical volume
// for which the value of label.time_map_updated and the value of 
// lable.time_unmounted are not equal, then the volume was improperly
// shutdown.

    word36 time_map_upd = extr36 (r0, label_time_map_updated_os);
    word36 time_unmounted = extr36 (r0, label_time_unmounted_os);

    //fprintf (stderr, "Time map updated %012lo\n", time_map_upd);
    //fprintf (stderr, "Time unmounted   %012lo\n", time_unmounted);

    if (time_map_upd != time_unmounted)
      fprintf (stderr, "WARNING: Not dismounted properly\n");

    //fprintf (stderr, "pack PVID %012lo\n", extr36 (r0, label_perm_os + 33));
    //fprintf (stderr, "pack LVID %012lo\n", extr36 (r0, label_perm_os + 34));

#if 0
// What is the root VTOC index?

    word36 root_votc = extr36 (r0, label_root_os + 0);
    if (root_votc & 0400000000000lu)
      {
        root_votc &= 0377777777777lu;
        fprintf (stderr, "Root is on this pack; VOTC index of %012lo (%lu)\n", root_votc, root_votc);
      }
#endif

#if 0
    word36 vtoc_size = extr36 (r0, label_perm_os + 39);
    fprintf (stderr, "vtoc_size %lu\n", vtoc_size);
#endif

// Lack of understanding. According to ddl the VTOC region is first record 8 (10o)  size 7487 (16477o)  
#if 0
// Where be the VTOC?

    word36 vtoc_origin = extr36 (r0, label_vtoc_origin_record_os);
    fprintf (stderr, "VTOC origin %012lo\n", vtoc_origin);
    for (int i = label_root_os; i < label_root_os + 20; i ++)
      {
        word36 w = extr36 (r0, i);
        fprintf (stderr, "012lo %s\n", w, str (w));
      }
#else
    vtoc_origin = 8;
    vtoc_header = 4;
#endif

    record vtoch;
    memset (& vtoch, 0, sizeof (record));
    readRecord (fd, vtoc_header, & vtoch);
    //word36 n_vtoces = extr36 (vtoch, vtoc_header_n_vtoce_os);
    //word36 n_free_vtoces = extr36 (vtoch, vtoc_header_n_free_vtoce);
    word36 vtoc_last_recno = extr36 (vtoch, vtoc_header_vtoc_last_recno);
    //fprintf (stderr, "n_vtoces %lu\n", n_vtoces);
    
    word36 vtoc_sz_recs = vtoc_last_recno + 1 - vtoc_origin;
    m_data ->  vtoc_no = (int) (vtoc_sz_recs * 2);
    //fprintf (stderr, "vtoc_no %lu\n", vtoc_no);

    m_data -> uid_table = calloc (sizeof (word36), m_data -> vtoc_no);
    if (m_data -> uid_table == NULL)
      {
        perror ("uid_table alloc");
        abort ();
      }
    
    m_data -> name_table = calloc (sizeof (char *),  m_data -> vtoc_no);
    if (m_data -> name_table == NULL)
      {
        perror ("name_table alloc");
        abort ();
      }
    
    m_data -> fq_name_table = calloc (sizeof (char *),  m_data -> vtoc_no);
    if (m_data -> fq_name_table == NULL)
      {
        perror ("fq_name_table alloc");
        abort ();
      }
    
    m_data -> attr_table = calloc (sizeof (word36),  m_data -> vtoc_no);
    if (m_data -> attr_table == NULL)
      {
        perror ("attr_table alloc");
        abort ();
      }



// Build uid, attr  and name table

    //word36 free = 0;
    //word36 found = 0;
    for (int i = 0; i < m_data -> vtoc_no; i ++)
      {
        VTOCE vtoce;
        readVTOCE (fd, i, & vtoce);
        word36 uid = vtoce [1];
        m_data -> uid_table [i] = uid;
        m_data -> attr_table [i] = vtoce [5];
        if (! uid)
          {
            m_data -> name_table [i] = NULL;
            //free ++;
            continue;
          }

        if (uid == 0777777777777lu) // root
          {
            m_data -> name_table [i] = strdup (">");
            //free ++;
            continue;
          }

        word36 path_uid = vtoce [160];
        if (path_uid != 0777777777777lu)
          {
            m_data -> uid_table [i] = 0;
            m_data -> name_table [i] = NULL;
            //free ++;
            continue;
          }

//fprintf (stderr, "04u %012lo\n", i, uid);
        char name [33 + 100];
        name [0] = 0;
        for (int j = 0; j < 8; j ++)
           strcat (name, str (vtoce [vtoce_primary_name_os + j]));
        for (int j = strlen (name) - 1; j >= 0; j --)
           if (name [j] == ' ')
             name [j] = 0;
           else
             break;
        m_data -> name_table [i] = strdup (name);
        //found ++;
      }

// Build fq_name table

    for (int i = 0; i < m_data -> vtoc_no; i ++)
      {
        if (! m_data -> uid_table [i])
          continue;
        char fq_name [4096];
        fq_name [0] = 0;

        VTOCE vtoce;
        readVTOCE (fd, i, & vtoce);
        for (int j = 0; j < 16; j ++)
          {
            word36 path_uid = vtoce [160 + j];
            if (! path_uid)
              break;
            for (int k = 0; k < m_data -> vtoc_no; k ++)
              {
                if (m_data -> uid_table [k] == path_uid)
                  {
                    strcat (fq_name, m_data -> name_table [k]);
                    if (j)
                      strcat (fq_name, ">");
                    break;
                  }
              }
          }

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
        m_data -> fq_name_table [i] = strdup (fq_name);
        //printf ("%4d  (%s)\n", i, m_data -> fq_name_table [i]);
        //log_msg ("%4d  ", i);
        //log_msg (" (%s)\n", fq_name);
      }

    return 0;
  }

int find_uid (word36 uid)
  {
    int vtoc_no = M_DATA -> vtoc_no;
    word36 * utab = M_DATA -> uid_table;
    for (int i = 0; i < vtoc_no; i ++)
      if (utab [i] == uid)
        return i;
    return -1;
  }

#if 0
int find_entry (char * name, word36 * upath, int npath)
  {
    for (uint i = 0; i < vtoc_no; i ++)
      {
        VTOCE vtoce;
        readVTOCE (fd, i, & vtoce);
        word36 uid = vtoce [1];
        if (uid)
          {
            int cnt;
            for (cnt = 0; cnt < 16; cnt ++)
              {
                word36 path_uid = vtoce [160 + cnt];
                if (! path_uid)
                  break;
              }
            if (cnt != npath) // different path lengths
              goto next;
            for (int j = 0; j < npath; j ++)
              {
                word36 path_uid = vtoce [160 + cnt];
                if (path_uid != upath [npath])
                  goto next;
              }
            char ent_name [33];
            ent_name [0] = 0;
            for (int j = 0; j < 8; j ++)
               strcat (ent_name, str (vtoce [vtoce_primary_name_os + j]));
            for (int j = strlen (ent_name) - 1; j >= 0; j --)
               if (name [j] == ' ')
                 name [j] = 0;
               else
                 break;
            if (strcmp (ent_name, name) == 0)
              return i;
          }
next: ;
      }

  }
#endif

void fixit (char * s)
  {
    size_t l = strlen (s);
    for (size_t i = 0; i < l; i ++)
      if (s [i] == '/')
        s [i] = '>';
  }

// return index into uid table; -1 if no such file or directory
int mx_lookup_path (const char * path)
  {
    struct m_state * m_data = M_DATA;

    log_msg ("mx_lookup_path %s\n", path);
    if (path [0] != '/')
      {
        log_msg ("mx_lookup_path not at root (%s)\n", path);
        return -1;
      }
    char * s = strdup (path);
    fixit (s);
    for (int i = 0; i < m_data -> vtoc_no; i ++)
      {
        if (! m_data -> uid_table [i])
          continue;
//log_msg ("%s %s\n", s, m_data -> fq_name_table [i]);
        if (strcmp (s, m_data -> fq_name_table [i]) == 0)
          return i;
      }
    return -1;
  }

// search for entries that are in the directory 'path'
int mx_readdir (off_t offset, const char * path)
  {
    struct m_state * m_data = M_DATA;
    char * s = strdup (path);
    fixit (s);
    size_t sl = strlen (s);
    if (s [sl -1] != '>')
      {
        strcat (s, ">");
        sl ++;
      }
//printf ("mx_readdir fixit [%s]\n", s);
    for (int i = offset; i < m_data -> vtoc_no; i ++)
      {
        if (! m_data -> uid_table [i])
          continue;

        char * fq = m_data -> fq_name_table [i];
        size_t fql = strlen (fq);

        // Find the rightmost > in fq_name
        char * last = strrchr (fq, '>');
        if (! last)
          {
            printf ("not last? %s\n", fq);
            continue;
          }
        size_t dir_len = last - fq + 1;
        if (dir_len != sl)
          continue;
        if (fql <= sl)
          continue;
        if (strncmp (s, fq, sl) == 0)
          {
//printf ("mx_readdir %ld %s %s\n", sl, s, fq);
            return i;
          }
      }
    return -1;
  }
