#######################################################################
#
#  Makefile for the fibonacci example application
#
#######################################################################

MWDIR=..
MWFILEDIR=$(MWDIR)/FileRC
PRESENTDIR=example-app

all:
	make -f Makefile.file.new
clean:
	rm -f *.o core
	@echo " *** You're clean now"

reallyclean: clean
	rm -f $(MASTER_NAME) $(WORKER_NAME) $(DOCS) out_* work.log
	cd $(MWDIR); make clean; cd $(PRESENTDIR)
	cd $(MWFILEDIR); make clean; cd $(MWDIR); cd $(PRESENTDIR)
	@echo " *** You're really clean now."

myclean:
	/bin/rm ./submit_files/* ./worker_input/* ./worker_output/* core*
