#######################################################################
#
#  Makefile for MW and FATCOP IP solver
#
#######################################################################

#
#  What arch are you compiling for?  I need this for both
#   the libraries to link in and for PVM
#
#  ARCHOS is SUN4SOL2 or X86SOL2 or LINUX
# 

ARCHOS = SUN4SOL2
ARCHLIB	= -lsocket -lnsl

CC=g++
CFLAGS= -O#g -ggdb -Wall 
CDEFINES= #-DSTANDALONE\
	#-DDEBUG_PRIMAL\
	#-DDEBUG_CUTS\
	#-DDEBUGMW\
	#-DDEBUG_DFS\
	#-DDEBUG_PSEUDO\
	#-DDEBUG_BOUNDS  -DDEBUG_CUTS2 #-DDUMB_COMPILER

PURIFY=#purify

# Just master objects
MWMOBJS = ../MWDriver.o \
	../MWTask.o \
	../MWWorkerID.o \
	../MWStats.o \
	../MWprintf.o\
	../MWPvmRC.o

# just worker objects
MWWOBJS = ../MWTask.o \
	../MWWorker.o \
	../MWprintf.o\
	../MWPvmRC.o\
	../MWWorkerID.o

MWOBJS = $(MWMOBJS) $(MWWOBJS)

MASTER_NAME     = fatcop-driver
WORKER_NAME     = fatcop-worker

DOC_TARGET_DIR  = ./html/

APP_HEADERS     = # Put these in later

MASTERSOURCES   = fatcop.cc\
	FATCOP_driver.cc\
	FATCOP_task.cc\
	cproblemspec.cc\
	cutpool.cc\
	cuts.cc\
	heuristics.cc\
	mipnode.cc\
	pseudocost.cc\
	presolve.cc

WORKERSOURCES   = fatcop_slave.cc\
	FATCOP_worker.cc\
	FATCOP_task.cc\
	branching.cc\
	cproblemspec.cc\
	cutpool.cc\
	cuts.cc\
	diving.cc\
	flowcov.cc\
	heuristics.cc\
	knapsack.cc\
	mipnode.cc\
	pseudocost.cc\
	presolve.cc\
	sknapsack.cc

MASTEROBJS      = $(MASTERSOURCES:.cc=.o)
WORKEROBJS      = $(WORKERSOURCES:.cc=.o)

DOCDIR          = /unsup/doc++/bin
DOCOPTIONS      = -p
DOCS            = $(APP_HEADERS:.h=.doc)

# PVM things:
#  (If STANDALONE, then make sure that PVM_ROOT is the path to a
#   "normal" PVM release)
PVM_ROOT = 	/p/condor/workspaces/pvm/pvm3.4.2
#PVM_ROOT =	/local.wheel/linderot/pvm3
#PVM_ROOT =	/local.vulture/metaneos/src/pvm3
#PVM_ROOT = 	/local.monterey6/testneos/jeff/src/pvm3
#PVM_ROOT = 	/software/common/apps/packages/pvm3

#PVM_ARCH =	X86SOL2
PVM_ARCH = 	$(ARCHOS)
ARCHCFLAGS	= 

# There are the needed for solaris
PVM_INCLS = -I$(PVM_ROOT)/include
PVM_LIB = -L$(PVM_ROOT)/lib/$(PVM_ARCH) -lpvm3 $(ARCHLIB) -lsocket -lnsl

DEPGEN=	makedepend
# (Need this so that makedepend doesn't complain)
#CCINCLS= -I/s/gcc/include/g++ -I/s/gcc/@sys/i386-pc-solaris2.6/include
CCINCLS= -I/s/gcc/include/g++ -I/s/gcc/@sys/sparc-sun-solaris2.6/include
#CCINCLS = -I/software/solaris-2/gnu/packages/libg++-2.7.2/lib/g++-include 


#
# Some stuff for the LPSOLVER library
#

LPSOLVERDIR=/p/condor/workspaces/shared.1/metaneos/linderot/LPSOLVER
#LPSOLVERDIR=/local.wheel/linderot/src/LPSOLVER
#LPSOLVERDIR=/local.vulture/metaneos/src/LPSOLVER
#LPSOLVERDIR=/local.monterey6/testneos/jeff/src/LPSOLVER
#LPSOLVERDIR=/home/linderot/src/LPSOLVER

LPSOLVER1= SOPLEXv1
LPSOLVER2= CPLEXv6

SOPLEXDIR=/p/condor/workspaces/shared.1/metaneos/linderot/soplex11
#SOPLEXDIR=/local.wheel/linderot/src/soplex11
#SOPLEXDIR=/local.vulture/metaneos/src/soplex11
#SOPLEXDIR=/local.monterey6/testneos/jeff/src/soplex11
#SOPLEXDIR=/home/linderot/src/soplex
CPLEXDIR=/s/cplex-6.5

LIBDIR = /p/condor/workspaces/shared.1/metaneos/linderot/lib/SUN4SOL2/

LPDEFINES= -D$(LPSOLVER1) -D$(LPSOLVER2)
LPINCLS= -I$(LPSOLVERDIR) -I$(SOPLEXDIR) -I$(CPLEXDIR)/include 
LPLIBS= -L$(LPSOLVERDIR)/../lib/$(ARCHOS) -llpsolver-$(LPSOLVER1)\
	-llpsolver-$(LPSOLVER2) \
	-L$(SOPLEXDIR)/../lib/$(ARCHOS) -lsoplex -L$(CPLEXDIR)/lib/ -lcplex -lm

#
# Put everything together
#

INCLS=	$(CCINCLS) -I./ -I../ $(PVM_INCLS) $(SPINCLS) $(LPINCLS) 
LIBS=	$(PVM_LIB) $(SPLIBS) $(LPLIBS) 
DEFINES= $(CDEFINES) $(LPDEFINES)

###########################################################################

all: depend master worker

master: $(MWMOBJS) $(MASTEROBJS)
	$(PURIFY) $(CC) $(CFLAGS) -o $(MASTER_NAME) \
		$(MWMOBJS) $(MASTEROBJS) $(LIBS)
	@echo " *** FATCOP Driver created ***"
	#mv $(MASTER_NAME) submit

worker: $(MWWOBJS) $(WORKEROBJS)
	$(PURIFY) $(CC) $(CFLAGS) -o $(WORKER_NAME)-$(ARCHOS) \
		$(MWWOBJS) $(WORKEROBJS) $(LIBS)
	@echo " *** FATCOP Worker created ***"
	#mv $(WORKER_NAME)-$(ARCHOS) submit

depend:
	$(DEPGEN) $(INCLS) $(CCINCLS) $(DEFINES) $(ALLSOURCES)
	@echo " *** Done making depend"

clean:
	rm -f $(MASTEROBJS) $(WORKEROBJS) $(MWOBJS) core
	@echo " *** You're clean now"

reallyclean: clean
	rm -f $(MASTER_NAME) $(WORKER_NAME) $(DOCS)
	@echo " *** You're really clean now."

doc: $(DOCS)
	$(DOCDIR)/doc++ -d $(DOC_TARGET_DIR) \
		$(DOCOPTIONS) $(DOCS); \
	rm -f $(DOCS)
	@echo " *** Made doc++"

######################################################################

.SUFFIXES: .o .C .cc .doc

.C.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLS) -c -o $(<:.C=.o) $<

.cc.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLS) -c -o $(<:.cc=.o) $<

.h.doc:
	$(DOCDIR)/docify $< > $(<:h=doc)


#
# DO NOT DELETE THIS LINE -- make depend depends on it.
