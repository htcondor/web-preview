# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  Makefile for the fibonacci example application
#
#######################################################################

# Turn off default suffixes
.SUFFIXES:
.SUFFIXES: .C .o

SHELL = /bin/sh

prefix = /p/condor/workspaces/sanjeevk/trunk/src/mw
exec_prefix = ${prefix}
includedir = @include_dir@
libdir = ${exec_prefix}/lib
bindir = ${exec_prefix}/bin
srcdir = .
top_srcdir = ../..

CXX = c++
CPPFLAGS = 
CXXFLAGS = -g -O2
LDFLAGS = 
LIBS = -lnsl 
INSTALL = /usr/bin/install -c

srcdir = .
top_srcdir = ../..


MWDIR=/p/condor/workspaces/sanjeevk/trunk/src/mw
#MWDIR=/u/l/i/linderot/afs/LINUX/MW

# The user puts this or configure -- It would be nice if configure did it.
CONDOR_DIR = /unsup/condor
CONDORPVM_DIR = /p/condor/workspaces/pvm/pvm3.4.2
PVM_ARCH = LINUX
SOCKET_LIB = -lnsl

CXX = g++
CXXFLAGS = -g
INCL = -I$(MWDIR)/src -I$(MWDIR)/src/RMComm


# Now some macros
MASTERSRC   = Driver-fib.C Task-fib.C MasterMain-fib.C
WORKERSRC   = Worker-fib.C Task-fib.C WorkerMain-fib.C

MASTEROBJ      = $(MASTERSRC:.C=.o)
WORKEROBJ      = $(WORKERSRC:.C=.o)

# Now the targets

all: file-based	condorpvm-based

file-based: filemaster fileworker

condorpvm-based: condorpvmmaster condorpvmworker

filemaster: $(MASTEROBJ)
	$(CXX) -o masterfib-file $(CXXFLAGS) $(LDFLAGS) $(MASTEROBJ) \
	-L$(MWDIR)/lib   -lMWfilemaster -lMW /unsup/condor/lib/libcondorapi.a 

fileworker: $(WORKEROBJ)
	condor_compile $(CXX) -o workerfib-file $(CXXFLAGS) $(LDFLAGS) $(WORKEROBJ) \
		-L$(MWDIR)/lib -lMWfileworker  -lMW  -L$(CONDOR_DIR)/lib -lcondorapi

condorpvmmaster: $(MASTEROBJ)
	$(CXX) -o masterfib-condorpvm $(MASTEROBJ) \
	-L$(MWDIR)/lib   -lMWcondorpvmmaster -lMW \
	-L$(CONDORPVM_DIR)/lib/$(PVM_ARCH) -lpvm3 $(SOCKET_LIB)

condorpvmworker: $(WORKEROBJ)
	$(CXX) -o workerfib-condorpvm $(WORKEROBJ) \
	-L$(MWDIR)/lib   -lMWcondorpvmworker -lMW \
	-L$(CONDORPVM_DIR)/lib/$(PVM_ARCH) -lpvm3 $(SOCKET_LIB)

clean:
	rm -f $(MASTEROBJ) $(WORKEROBJ)


# default rule
.C.o:
	${CXX} -c ${CPPFLAGS} ${CXXFLAGS} $(INCL) $<
