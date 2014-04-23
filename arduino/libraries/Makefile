#
# This makefile is only used to run Doxygen
#

# unconditionally run Doxygen and latex to get doc/latex/refman.pdf
all:
	doxygen

pdf:
	doxygen
	cd doc/latex ; make

# remove documentation directory
clean:
	rm -rf doc
