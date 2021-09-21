# Generated automatically from Makefile.in by configure.
#######################################################################
#
#  Makefile for the matrix multiplication example application
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

# The user puts this or configure -- It would be nice if configure did it.
CONDOR_DIR = /unsup/condor
CONDORPVM_DIR = /p/condor/workspaces/pvm/pvm3.4.2
PVM_ARCH = LINUX
#PVM_ARCH = X86SOL2
#SOCKET_LIB = -lsocket -lnsl
SOCKET_LIB = -lnsl

CXX = g++
CXXFLAGS = -g
INCL = -I$(MWDIR)/src -I$(MWDIR)/src/RMComm


# Now some macros
MASTERSRC   = Driver-Matmul.C Task-Matmul.C MasterMain-Matmul.C
WORKERSRC   = Worker-Matmul.C Task-Matmul.C WorkerMain-Matmul.C

MASTEROBJ      = $(MASTERSRC:.C=.o)
WORKEROBJ      = $(WORKERSRC:.C=.o)

# Now the targets

all:	file-based condorpvm-based

file-based: filemaster fileworker

condorpvm-based: condorpvmmaster condorpvmworker

filemaster: $(MASTEROBJ)
	$(CXX) -o mastermatmul-file $(CXXFLAGS) $(LDFLAGS) $(MASTEROBJ) \
	-L$(MWDIR)/lib   -lMWfilemaster -lMW /unsup/condor/lib/libcondorapi.a

fileworker: $(WORKEROBJ)
	condor_compile $(CXX) -o workermatmul-file $(CXXFLAGS) $(LDFLAGS) $(WORKEROBJ) \
		-L$(MWDIR)/lib -lMWfileworker  -lMW /unsup/condor/lib/libcondorapi.a 

condorpvmmaster: $(MASTEROBJ)
	$(CXX) -o mastermatmul-condorpvm $(MASTEROBJ) \
	-L$(MWDIR)/lib   -lMWcondorpvmmaster -lMW \
	-L$(CONDORPVM_DIR)/lib/$(PVM_ARCH) -lpvm3 $(SOCKET_LIB)

condorpvmworker: $(WORKEROBJ)
	$(CXX) -o workermatmul-condorpvm $(WORKEROBJ) \
	-L$(MWDIR)/lib   -lMWcondorpvmworker -lMW \
	-L$(CONDORPVM_DIR)/lib/$(PVM_ARCH) -lpvm3 $(SOCKET_LIB)

clean:
	rm -f $(MASTEROBJ) $(WORKEROBJ)


# default rule
.C.o:
	${CXX} -c ${CPPFLAGS} ${CXXFLAGS} $(INCL) $<
