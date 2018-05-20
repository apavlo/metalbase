#
#  METALBASE 5.0
#
#  Released October 1st, 1992 by Huan-Ti [ virtual!root@owlnet.rice.edu ]
#                                        [ t-richj@microsoft.com ]
#
#  Makefile for demonstration program  -- *nix expected
#
#

CFLAGS= -I/usr/local/include

all : sample bench

clean:
	rm -f *.o bench sample

bench : bench.o
	cc -f -o bench bench.o -lmb

bench.o : bench.c bench.h
	cc -c $(CFLAGS) bench.c

bench.h : bench.s
	build -qh bench.s

sample : sample.o
	cc -f -o sample sample.o -lmb -lncurses

sample.o : sample.c sample.h sample_fm.h
	cc -c $(CFLAGS) sample.c

sample.h : sample.s
	build -qh sample.s

sample_fm.h : sample.frm
	form sample

