# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  Makefile for MW sources
#
#######################################################################

# Turn off default suffixes
.SUFFIXES:
.SUFFIXES: .C .o

SHELL = /bin/sh

INCLUDES = -I./ -I../ -I./RMComm -I./MWControlTasks

prefix = /usr/local
exec_prefix = ${prefix}
includedir = @include_dir@
libdir = ${exec_prefix}/lib
bindir = ${exec_prefix}/bin
srcdir = .
top_srcdir = ..

CXX = g++
CPPFLAGS =  $(INCLUDES)
CXXFLAGS = -g -O2
LDFLAGS = 
DEFS =  -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_VPRINTF=1 -DHAVE_GETCWD=1 -DHAVE_GETHOSTNAME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_MKDIR=1 -DHAVE_STRSTR=1 -DHAVE_DYNAMIC_CAST= 
LIBS = 
INSTALL = .././install-sh -c
DOCXX = doc++

srcdir = .
top_srcdir = ..



# Define some macros
MWMASTERSRC = ${srcdir}/MWDriver.C \
	${srcdir}/MWTask.C \
	$(srcdir)/MWControlTasks/MWNWSTask.C \
	${srcdir}/MWWorkerID.C \
	${srcdir}/MWStats.C \
	${srcdir}/MWprintf.C

MWMASTEROBJ = $(MWMASTERSRC:.C=.o)

MWWORKERSRC = ${srcdir}/MWWorker.C \
	${srcdir}/MWTask.C \
	$(srcdir)/MWControlTasks/MWNWSTask.C \
	${srcdir}/MWprintf.C \
	${srcdir}/MWWorkerID.C

MWWORKEROBJ = $(MWWORKERSRC:.C=.o)

MWHEADERS = ${srcdir}/MW.h \
	${srcdir}/MWDriver.h \
	${srcdir}/MWStats.h \
	${srcdir}/MWTask.h \
	$(srcdir)/MWControlTasks/MWNWSTask.h \
	${srcdir}/MWWorker.h \
	${srcdir}/MWWorkerID.h \
	${srcdir}/MWprintf.h

#
#

# Now the targets
all : mw mwfile mwcondorpvm


mw : $(MWMASTEROBJ) $(MWWORKEROBJ)
	mv -f MWNWSTask.o MWControlTasks/
	ar cr libMW.a $(MWMASTEROBJ) $(MWWORKEROBJ); ranlib libMW.a

mwfile: 
	cd ${srcdir}/RMComm/MW-File; make all; cd ${srcdir}

mwcondorpvm:
	cd ${srcdir}/RMComm/MW-CondorPVM; make all; cd ${srcdir}

clean:
	rm -f $(MWMASTEROBJ) $(MWWORKEROBJ)
	cd ${srcdir}/RMComm/MW-File; make clean; cd ${srcdir}
	cd ${srcdir}/RMComm/MW-CondorPVM; make clean; cd ${srcdir}

distclean: clean
	rm -f *~ Makefile	

# default rule
.C.o:
	${CXX} -c ${DEFS} ${CPPFLAGS} ${CXXFLAGS} $<

