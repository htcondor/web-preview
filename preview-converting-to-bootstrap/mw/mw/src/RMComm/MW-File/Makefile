# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  Makefile for Condor-FileBased RMComm 
#
#######################################################################

# Turn off default suffixes
.SUFFIXES:
.SUFFIXES: .C .o

SHELL = /bin/sh

prefix = /usr/local
exec_prefix = ${prefix}
includedir = @include_dir@
libdir = ${exec_prefix}/lib
bindir = ${exec_prefix}/bin
srcdir = .
top_srcdir = ../../..

CXX = g++
CPPFLAGS = 
CXXFLAGS = -g -O2
LDFLAGS = 
LIBS = 
DEFS =  -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_VPRINTF=1 -DHAVE_GETCWD=1 -DHAVE_GETHOSTNAME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_MKDIR=1 -DHAVE_STRSTR=1 -DHAVE_DYNAMIC_CAST= 

srcdir = .
top_srcdir = ../../..

CONDOR_DIR = /unsup/condor
MW_DIR = ${top_srcdir}/src

MASTERSRC = ${srcdir}/../MWRMComm.C \
	${srcdir}/MWFileRC.C \
	${srcdir}/list.C

# Note that we use a compiler define to distinguish between master
# and worker -- so we have an awkward naming convention here
# Also, we put the (abstract) RMComm object file in the current directory
MASTEROBJ = MWRMComm.o \
	MWFileRCM.o \
	list.o

WORKERSRC = ${srcdir}/../MWRMComm.C \
	${srcdir}/MWFileRC.C \
	${srcdir}/list.C

# Note that we use a compiler define to distinguish between master
# and worker -- so we have an awkward naming convention here
WORKEROBJ = MWRMComm.o \
	MWFileRCW.o \
	list.o


#
# Put everything together
#

INCLS=	-I../ -I$(MW_DIR) -I$(CONDOR_DIR)/include

all: mwfilemaster mwfileworker

mwfilemaster: $(MASTEROBJ)
	ar cr libMWfilemaster.a $(MASTEROBJ)
	ranlib libMWfilemaster.a

mwfileworker: $(WORKEROBJ)
	ar cr libMWfileworker.a $(WORKEROBJ)
	ranlib libMWfileworker.a

clean:
	rm -f $(MASTEROBJ) $(WORKEROBJ)
	@echo " *** You're clean now"

distclean: clean
	rm -f *~ Makefile libMWfilemaster.a libMWfileworker.a


MWFileRCM.o: MWFileRC.C MWFileRC.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLS) -DFILE_MASTER  -c MWFileRC.C -o MWFileRCM.o

MWFileRCW.o: MWFileRC.C MWFileRC.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLS) -c MWFileRC.C -o MWFileRCW.o

MWRMComm.o: ../MWRMComm.C ../MWRMComm.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLS) -c ../MWRMComm.C

.C.o:
	${CXX} -c ${DEFS} ${CPPFLAGS} ${CXXFLAGS} $(INCLS) $<

