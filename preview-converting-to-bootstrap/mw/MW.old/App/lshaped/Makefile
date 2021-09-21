#######################################################################
#
#  Makefile for LShaped
#
#######################################################################

CC=g++
CFLAGS= -g -ggdb -Wall
CDEFINES= #-DSTANDALONE #-DDUMB_COMPILER

PURIFY=#purify

# Just master objects
MWMOBJS = ../MWDriver.o \
	../MWTask.o \
	../MWWorkerID.o \
	../MWStats.o \
	../MWprintf.o

# just worker objects
MWWOBJS = ../MWTask.o \
	../MWWorker.o \
	../MWprintf.o

MWOBJS = $(MWMOBJS) $(MWWOBJS)

MASTER_NAME     = lshaped-driver
WORKER_NAME     = lshaped-worker

DOC_TARGET_DIR  = ./html/

APP_HEADERS     = lshaped-driver.h lshaped-driverstats.h \
                  lshaped-task.h lshaped-worker.h

MASTERSOURCES   = lshaped-driver.C lshaped-driverstats.C \
                  lshaped-task.C lshaped-driver-main.C

WORKERSOURCES   = lshaped-worker.C lshaped-task.C lshaped-worker-main.C


MASTEROBJS      = $(MASTERSOURCES:C=o)
WORKEROBJS      = $(WORKERSOURCES:C=o)

DOCDIR          = /unsup/doc++/bin
DOCOPTIONS      = -p
DOCS            = $(APP_HEADERS:.h=.doc)

# PVM things:
PVM_ROOT = 	/p/condor/workspaces/pvm/pvm3.4.1
#PVM_ROOT =	/local.wheel/linderot/pvm3
#PVM_ROOT = 	/local.monterey6/testneos/jeff/src/pvm3
#PVM_ROOT = 	/software/common/apps/packages/pvm3

PVM_ARCH =	X86SOL2
#PVM_ARCH = 	SUN4SOL2
ARCHCFLAGS	= 

# There are the needed for solaris
ARCHLIB	= -lsocket -lnsl
PVM_INCLS = -I$(PVM_ROOT)/include
PVM_LIB = -L$(PVM_ROOT)/lib/$(PVM_ARCH) -lpvm3 $(ARCHLIB)

DEPGEN=	makedepend
# (Need this so that makedepend doesn't complain)
CCINCLS= -I/s/gcc/include/g++ -I/s/gcc/@sys/i386-pc-solaris2.6/include
#CCINCLS= -I/s/gcc/include/g++ -I/s/gcc/@sys/sparc-sun-solaris2.6/include
#CCINCLS = -I/software/solaris-2/gnu/packages/libg++-2.7.2/lib/g++-include 

#
# Some stuff for your SPUTIL library
#

#SPUTILDIR = /local.wheel/linderot/src/SUTIL
SPUTILDIR = /local.vulture/metaneos/src/SUTIL
#SPUTILDIR = /local.monterey6/testneos/jeff/src/SUTIL
#SPUTILDIR = /home/linderot/src/SUTIL

SPDEFINES= -DUSING_PVM
SPINCLS= -I$(SPUTILDIR)
SPLIBS= -L$(SPUTILDIR) -lsputil

#
# Some stuff for the LPSOLVER library
#

LPSOLVER= SOPLEXv1
#SOPLEXDIR=/local.wheel/linderot/src/soplex11
SOPLEXDIR=/local.vulture/metaneos/src/soplex11
#SOPLEXDIR=/local.monterey6/testneos/jeff/src/soplex11
#SOPLEXDIR=/home/linderot/src/soplex
#LPSOLVERDIR=/local.wheel/linderot/src/LPSOLVER
LPSOLVERDIR=/local.vulture/metaneos/src/LPSOLVER
#LPSOLVERDIR=/local.monterey6/testneos/jeff/src/LPSOLVER
#LPSOLVERDIR=/home/linderot/src/LPSOLVER

LPDEFINES= -D$(LPSOLVER)
LPINCLS= -I$(LPSOLVERDIR) -I$(SOPLEXDIR) 
LPLIBS= -L$(LPSOLVERDIR) -llpsolver-$(LPSOLVER) -L$(SOPLEXDIR) -lsoplex -lm

#
# Put everything together
#

INCLS=	$(CCINCLS) -I./ -I../ $(PVM_INCLS) $(SPINCLS) $(LPINCLS) 
LIBS=	$(PVM_LIB) $(SPLIBS) $(LPLIBS)
DEFINES= $(CDEFINES) $(LPDEFINES) $(SPDEFINES) 

###########################################################################

all: depend master worker

master: $(MWMOBJS) $(MASTEROBJS)
	$(PURIFY) $(CC) $(CFLAGS) -o $(MASTER_NAME) \
		$(MWMOBJS) $(MASTEROBJS) $(LIBS)
	@echo " *** LShaped Driver created ***"

worker: $(MWWOBJS) $(WORKEROBJS)
	$(CC) $(CFLAGS) -o $(WORKER_NAME) \
		$(MWWOBJS) $(WORKEROBJS) $(LIBS)
	@echo " *** LShaped Worker created ***"

depend:
	$(DEPGEN) $(INCLS) $(CCINCLS) $(DEFINES) $(ALLSOURCES)
	@echo " *** Done making depend"

clean:
	rm -f $(MASTEROBJS) $(WORKEROBJS) core
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

.SUFFIXES: .o .C .doc

.C.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLS) -c -o $(<:C=o) $<

.h.doc:
	$(DOCDIR)/docify $< > $(<:h=doc)


#
# DO NOT DELETE THIS LINE -- make depend depends on it.
