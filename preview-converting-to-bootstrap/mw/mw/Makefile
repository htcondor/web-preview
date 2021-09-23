# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  The (base) Makefile for MW
#
#######################################################################

VERSION = 0.0.1


SHELL = /bin/sh

prefix = /usr/local
exec_prefix = ${prefix}
includedir = @include_dir@
libdir = ${exec_prefix}/lib
bindir = ${exec_prefix}/bin
srcdir = .
top_srcdir = .

CXX = g++
CPPFLAGS = 
CXXFLAGS = -g -O2
LDFLAGS = 
LIBS = 
INSTALL = ./install-sh -c

srcdir = .
top_srcdir = .


# Now the targets

all: source

source:
	cd ./src; make all; cd ..
	cp src/libMW.a lib/
	cp src/RMComm/*/lib* lib/

ex:
	cd ./examples/fib; make all; cd ../..

install: all install-directories
	echo "Copying libraries"
	cp -f ./src/libMW.a $(exec_prefix)/lib
	cp -f ./src/RMComm/MW-File/libMWfilemaster.a $(exec_prefix)/lib
	cp -f ./src/RMComm/MW-File/libMWfileworker.a $(exec_prefix)/lib
	cp -f ./src/RMComm/MW-CondorPVM/libMWcondorpvmmaster.a $(exec_prefix)/lib
	cp -f ./src/RMComm/MW-CondorPVM/libMWcondorpvmworker.a $(exec_prefix)/lib
	echo "Copying include files"
	cp -f src/MW.h src/MWDriver.h src/MWStats.h src/MWTask.h\
		src/MWWorker.h  src/MWWorkerID.h  src/MWprintf.h\
		src/RMComm/MWRMComm.h $(exec_prefix)/include

install-directories:
	$(SHELL) mkinstalldirs $(exec_prefix)/lib $(exec_prefix)/include

clean:
	cd src; make clean; cd ..
	cd ./examples/fib; make clean; cd ../..

distclean: clean
	rm -rf $(exec_prefix)/lib
	rm config.log config.cache Makefile
	rm *~

dist: configure clean
	cp -rp . /tmp/mw-$(VERSION)
	cd /tmp/mw-$(VERSION) ; tar cf - . | gzip -c > ../mw-$(VERSION).tar.gz
	rm -rf /tmp/mw-$(VERSION)

html:
	echo "Making DOC++"
	${DOCXX} -d ${top_srcdir}/doc/html `find . -name "*.h" -print`

# automatic re-running of configure if the configure.in file has changed
${srcdir}/configure: configure.in aclocal.m4
	cd ${srcdir} && autoconf

# autoheader might not change config.h.in, so touch a stamp file
${srcdir}/config.h.in: stamp-h.in
${srcdir}/stamp-h.in: configure.in aclocal.m4
	cd ${srcdir} && autoheader
	echo timestamp > ${srcdir}/stamp-h.in

config.h: stamp-h
stamp-h: config.h.in config.status
	./config.status
Makefile: Makefile.in config.status
	./config.status
config.status: configure
	./config.status --recheck
