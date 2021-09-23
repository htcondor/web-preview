#######################################################################
#
#  Makefile for the fibonacci example application
#
#######################################################################

MWDIR=../..
PRESENTDIR=example-app

#for pvm
VERS = pvm

#for file
VERS = file

#for Independent
VERS = ind

all:
	make -f Makefile.$(VERS)
clean:
	make -f Makefile.$(VERS) clean
	@echo " *** You're clean now"

reallyclean: 
	make -f Makefile.$(VERS) reallyclean
	@echo " *** You're really clean now."

myclean:
	make -f Makefile.$(VERS) myclean
