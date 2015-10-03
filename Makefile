#CC = gcc
#LD = gcc
CC = clang
LD = clang

CFLAGS  = -g -O0
CFLAGS += -std=c99 -U__STRICT_ANSI__
CFLAGS += -Wall 
CFLAGS += -Wunused-argument \
-Wunused-function \
-Wunused-label \
-Wunused-parameter \
-Wunused-value \
-Wunused-variable \
-Wunused \
-Wextra

mfs: mfs.c mfs.h mfslib.c mfslib.h
	$(CC) $(CFLAGS) `pkg-config fuse --cflags --libs` -o mfs mfs.c mfslib.c
