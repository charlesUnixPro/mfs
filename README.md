This is a User Space Filesystem to allow access to the contents
of Multics disk volumes from the host file system.

In order to build it, you will need your system's equivalent of the
fuse-devel package.

This code was developed on Fedora Linux, and has only been tested there.

This code only understands 3381 disk packs, as created by the DPS8-M
emulator.

Currently it is read-only; and only readdir is implemented. This means that
'ls' will read the volume directory, but the segments themselves are 
inaccessable; and the segment lengths, ACLs and AIMs are ignored.

To build:

~~~~
    $ make
~~~~

To run:

~~~~
    # Create a mount point
    $ mkdir mnt
    $ ./mfs rpv.dsk mnt
~~~~

To use:

~~~~
    $ ls mnt
    daemon_dir_dir   reload_dir                 system_library_standard
    disk_table       site                       system_library_tandd
    documentation    system_control_1           system_library_tools
    dumps            system_library_1           system_library_unbundled
    library_dir_dir  system_library_3rd_party   user_dir_dir
    lv               system_library_auth_maint
    process_dir_dir  system_library_obsolete
~~~~

To end:

~~~~
    $ fusermount -u mnt
~~~~

What happens if you mount a disk in use?

~~~~
    $ ./mfs rpv.dsk mnt
    WARNING: Not dismounted properly
~~~


