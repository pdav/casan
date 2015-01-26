#
# This file is only here to automate compilation and cleanup
# for testing purposes
#

SUBDIRS = test-coap test-l2addr test-l2net test-msg test-res test-time
SUBDIRS += test-zig
SUBDIRS += casan

all:
	for i in $(SUBDIRS) ; do \
	    ( cd $$i ; make clean ; make ) || exit 1 ; \
	done

clean:
	for i in $(SUBDIRS) ; do \
	    ( cd $$i ; rm -rf build-* ) ; \
	done
