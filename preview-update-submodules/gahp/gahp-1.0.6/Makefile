# To compile, set the correct PLATFORM_LIBS for your platform

CC = gcc

GLOBUS_PATH = /unsup/globus-2.2.2
FLAVOR = gcc32dbgpthr
GLOBUS_INCLUDE_PATH = $(GLOBUS_PATH)/include/$(FLAVOR)
GLOBUS_LIB_PATH = $(GLOBUS_PATH)/lib
GLOBUS_LIBS = $(GLOBUS_LIB_PATH)/libglobus_gass_server_ez_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gass_transfer_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gram_client_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gram_protocol_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_io_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gss_assist_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gssapi_gsi_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gsi_proxy_core_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gsi_credential_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gsi_callback_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_oldgaa_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gsi_sysconfig_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_proxy_ssl_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_gsi_cert_utils_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_openssl_error_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_openssl_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libssl_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libcrypto_$(FLAVOR).a $(GLOBUS_LIB_PATH)/libglobus_common_$(FLAVOR).a 

LINUX_LIBS = -lpthread
IRIX_LIBS = -lpthread
DUX_LIBS = -lpthread
SOLARIS_LIBS = -lpthread -lposix4 -lnsl -lsocket

PLATFORM_LIBS = $(LINUX_LIBS)

CFLAGS = -g -I$(GLOBUS_INCLUDE_PATH)

all: gahp_server

gahp_server: gahp_server.o
	$(CC) -o gahp_server gahp_server.o -L$(GLOBUS_LIB_PATH) $(GLOBUS_LIBS) $(PLATFORM_LIBS)

.c.o: 
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o gahp_server
