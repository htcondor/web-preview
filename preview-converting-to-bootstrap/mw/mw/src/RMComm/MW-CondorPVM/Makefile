# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  Makefile for Condor-PVM based RMComm 
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

#PVM_ROOT = /p/condor/workspaces/pvm/pvm3.4.2
PVM_ROOT = /p/condor/workspaces/pvm/pvm3.4.2

CXX = c++
CPPFLAGS =  -I../ -I../.. -I$(PVM_ROOT)/include
CXXFLAGS = -g -O2
LDFLAGS = 
LIBS = 
DEFS =  -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_LIMITS_H=1 -DHAVE_SYS_TIME_H=1 -DHAVE_UNISTD_H=1 -DTIME_WITH_SYS_TIME=1 -DHAVE_VPRINTF=1 -DHAVE_GETCWD=1 -DHAVE_GETHOSTNAME=1 -DHAVE_GETTIMEOFDAY=1 -DHAVE_MKDIR=1 -DHAVE_STRSTR=1 -DHAVE_DYNAMIC_CAST= 

srcdir = .
top_srcdir = ../../..



MASTERSRC = ${srcdir}/../MWRMComm.C \
	${srcdir}/MWCondorPvmRC.C

MASTEROBJ = MWRMComm.o\
	MWCondorPvmRCM.o

WORKERSRC = ../MWRMComm.C \
	MWCondorPvmRC.C

WORKEROBJ = MWRMComm.o\
	MWCondorPvmRCW.o


#
# The targets
#

all: mwcondorpvmmaster mwcondorpvmworker

mwcondorpvmmaster: $(MASTEROBJ)
	ar cr libMWcondorpvmmaster.a $(MASTEROBJ)
	ranlib libMWcondorpvmmaster.a

mwcondorpvmworker: $(WORKEROBJ)
	ar cr libMWcondorpvmworker.a $(WORKEROBJ)
	ranlib libMWcondorpvmworker.a

clean:
	rm -f $(MASTEROBJ) $(WORKEROBJ)

distclean:
	rm -f *~ Makefile libMWcondorpvmmaster.a libMWcondorpvmworker.a

MWCondorPvmRCM.o: MWCondorPvmRC.C MWCondorPvmRC.h
	$(CXX) $(DEFS) -DPVM_MASTER $(CPPFLAGS) $(CXXFLAGS) $(INCLS) -c MWCondorPvmRC.C -o MWCondorPvmRCM.o

MWCondorPvmRCW.o: MWCondorPvmRC.C MWCondorPvmRC.h
	$(CXX) $(DEFS) $(CPPFLAGS) $(CXXFLAGS) $(INCLS) -c MWCondorPvmRC.C -o MWCondorPvmRCW.o

MWRMComm.o: ../MWRMComm.C ../MWRMComm.h
	${CXX} ${DEFS} ${CPPFLAGS} ${CXXFLAGS} $(INCLS) -c ../MWRMComm.C

.C.o:
	${CXX} -c ${DEFS} ${CPPFLAGS} ${CXXFLAGS} $(INCLS) $<

