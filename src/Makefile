#
#  METALBASE 5.0
#
#  Released October 1st, 1992 by Huan-Ti [ richid@owlnet.rice.edu ]
#                                        [ t-richj@microsoft.com ]
#
#  Generic Makefile for 5.0 Library and Utilities
#
###############################################################################
#
# CFLAGS=
#    -DSTRUCT_1      -- Read lower for an explanation of these, and how to
#    -DSTRUCT_2      --    determine which is appropriate for your system.
#    -DSTRUCT_3
#    -DSTRUCT_4
#    -DLONGARGS      -- To produce ansi-style prototypes ("void fn(int)")
#    -DNOSYNC        -- Removes calls to sync() and fsync(), and in-line _asm
#    -DNOVOIDPTR     -- To use char* instead of void* (automatic for COHERENT)
#    -DNOENCRYPT     -- To remove encryption crap from library and utilities
#    -DNEED_USHORT   -- If your compiler doesn't have ushort yet (COH again)
#    -DNEED_ULONG    -- If your compiler doesn't have ulong yet (most don't)
#    -DUNIX_LOCKS    -- To enable Unix-style locking
#    -DSIG_TYPE=void -- void or int; needed only if you define UNIX_LOCKS
#    -DVI_EMU        -- To add vi emulation to input.c
#    -DMSDOS         -- MS-DOS users should define this if their CC doesn't.
#
# EXEDIR=            -- Directory where executables should go
# INCDIR=            -- Directory where include files should go
# LIBDIR=            -- Directory where libmb.a / mbase.lib should go
#
# LDOPTS=-f          -- To include floating point stuff for printf()
#
###############################################################################
#
# All users: Update the flags just below here FIRST (don't worry about
#            setting -DSTRUCT_?); then just type "make".  It will compile and
#            run struct/struct.exe, which will tell you how to determine how
#            -DSTRUCT_? should be set for your system.  Update this in the
#            Makefile and type "make install".  You may delete struct/
#            struct.exe after you've used it.
#
# DOS users: Try adding -DMSDOS to CFLAGS=; if you get a compiler error,
#            take it back out.  The code expects MSDOS to be defined for all
#            DOS compilers--most already set it, but some may not.
#
# Unix users: set -DUNIX_LOCKS to use flock() for file locking; otherwise,
#             MetalBase's inherent system will be used (which MAY cause
#             problems with code which does not exit properly, but which is
#             operationally identical).
#
###############################################################################

# defaults for Linux
EXEDIR=/usr/local/bin
INCDIR=/usr/local/include
LIBDIR=/usr/local/lib
OBJ=.o
LIB=libmb.a
CURSES= -lncurses 
NCURSES= -DNCURSES -I/usr/local/include
COPY=cp
CC=gcc
LDOPTS= -s

CFLAGS= -Wall -O $(NCURSES) -DSTRUCT_3 -DSIG_TYPE=void -DUSE_CURKEY -DNOENCRYPT -DVI_EMU -DLONGARGS -DUNIX_LOCKS -I. 

###############################################################################

BLAST   = blast
BUILD   = build
FORM    = form
MBCONV  = mbconv
REPORT  = report
VR      = vr
LIBRARY = libmb.a

HEADERS=stdinc.h mbase.h
TARGETS=$(BLAST) $(BUILD) $(FORM) $(MBCONV) $(REPORT) $(SAMPLE) $(VR)

###############################################################################

ARCHIVE = ar rv $(LIBRARY)
RANLIB  = ranlib $(LIBRARY)

###############################################################################

all: struct$(EXE) $(HEADERS) $(TARGETS)

install : all
	install -m 0755 -o bin -g bin $(TARGETS) $(EXEDIR)
	install -m 0644 -o bin -g bin mbase.h stdinc.h $(INCDIR)
	install -m 0644 -o bin -g bin $(LIBRARY) $(LIBDIR)

struct$(EXE) : struct$(OBJ)
	$(CC) -o $@ struct$(OBJ)
	@./struct
	@echo Now update the Makefile and make install

clean:
	rm -f *.o $(TARGETS) $(LIBRARY) 

###############################################################################

$(BLAST)   : blast$(OBJ)
	$(CC) -o $@ blast$(OBJ)

$(BUILD)   : build$(OBJ) $(LIBRARY)
	$(CC) $(LDOPTS) -o $@ build$(OBJ) $(LIBRARY)

$(FORM)    : form$(OBJ) form_wr$(OBJ) $(LIBRARY)
	$(CC) $(LDOPTS) -o $@ form$(OBJ) form_wr$(OBJ) $(LIBRARY)

$(MBCONV)  : mbconv$(OBJ) $(LIBRARY)
	$(CC) $(LDOPTS) -o $@ mbconv$(OBJ) $(LIBRARY)

$(REPORT)  : report$(OBJ) $(LIBRARY)
	$(CC) $(LDOPTS) -o $@ report$(OBJ) $(LIBRARY)

$(VR)      : vr$(OBJ) $(LIBRARY)
	$(CC) $(LDOPTS) -o $@ vr$(OBJ) $(LIBRARY) $(CURSES) 

$(LIBRARY) : entry$(OBJ) lock$(OBJ) input$(OBJ) mbase$(OBJ) parse$(OBJ) timedate$(OBJ) util1$(OBJ)\
util2$(OBJ) cache$(OBJ) create$(OBJ)
	$(ARCHIVE) entry$(OBJ) lock$(OBJ) input$(OBJ) mbase$(OBJ) parse$(OBJ) timedate$(OBJ)\
 util1$(OBJ) util2$(OBJ) cache$(OBJ) create$(OBJ)
	$(RANLIB)

