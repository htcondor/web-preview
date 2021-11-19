#######################################################################
#
#  Skeleton makefile
#
#######################################################################

CC=g++
CFLAGS= -g 
CDEFINES= 

PURIFY= #purify

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

# You'll want to give these a name...
MASTER_NAME     = my_master
WORKER_NAME     = my_worker

DOC_TARGET_DIR  = ./html/

# fill these in, too:
APP_HEADERS     = user_driver.h user_task.h user_worker.h
MASTERSOURCES   = user_driver.C user_task.C mastermain.C
WORKERSOURCES   = user_worker.C user_task.C workermain.C

MASTEROBJS      = $(MASTERSOURCES:C=o)
WORKEROBJS      = $(WORKERSOURCES:C=o)

# if you have doc++, provide the path:
DOCDIR          = 
DOCOPTIONS      = -p
DOCS            = $(APP_HEADERS:.h=.doc)

# PVM things:
# the path to your pvm installation directory:
PVM_ROOT = 	/p/condor/workspaces/pvm/pvm3.3.11
# the arch you're compiling for:
#PVM_ARCH = 	SUN4SOL2
PVM_ARCH = 	X86SOL2
ARCHCFLAGS	= 

# These are needed for solaris
ARCHLIB	= -lsocket -lnsl
PVM_INCLS = -I$(PVM_ROOT)/include
PVM_LIB = -L$(PVM_ROOT)/lib/$(PVM_ARCH) -lpvm3 $(ARCHLIB)

DEPGEN=	makedepend
# (Need this so that makedepend doesn't complain)
# Edit this as necedssary...
CCINCLS= -I/s/gcc/include/g++ -I/s/gcc/@sys/sparc-sun-solaris2.6/include

#
# Put everything together
#

INCLS=	$(PVM_INCLS) -I./ -I../
LIBS=	$(PVM_LIB)
DEFINES= $(CDEFINES)

all: depend master worker

master: $(MWOBJS) $(MASTEROBJS)
	$(CC) $(CFLAGS) -o $(MASTER_NAME) \
		$(MWOBJS) $(MASTEROBJS) $(LIBS)
	@echo " *** Master Fib. created ***"

worker: $(MWOBJS) $(WORKEROBJS)
	$(PURIFY) $(CC) $(CFLAGS) -o $(WORKER_NAME) \
		$(MWOBJS) $(WORKEROBJS) $(LIBS)
	@echo " *** Worker Fib. created ***"

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
# DO NOT DELETE
