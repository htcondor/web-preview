/*
 Copyright (c) 2002, University of Wisconsin-Madison Condor Project
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer. 

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution. 

 Neither the name of the University of Wisconsin, nor the Condor Project, nor
 any of its contributors may be used to endorse or promote products derived
 from this software without specific prior written permission. 
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include "globus_gram_client.h"
#include "globus_gass_server_ez.h"
#include "globus_gss_assist.h"

// WIN32 doesn't have strcasecmp
#ifdef WIN32
#define strcasecmp(s1, s2) stricmp(s1, s2)
#endif

	/* Define this if the gahp server should fork before dropping core */
#undef FORK_FOR_CORE

	// GSI is not thread safe.  In the Globus Toolkit, all
	// calls to GSI occur in Globus IO, and all entry points
	// to Globus IO are protected with a mutex.  
	// So, you _must_ call globus_i_io_mutex_lock() before
	// making a GSI call or the GRAM 1.6 proxy refresh call,
	// and of course you must call globus_i_io_mutex_unlock()
	// immediately upon the call's return.  -Todd <tannenba@cs.wisc.edu>
extern globus_mutex_t           globus_i_io_mutex;
extern volatile int         globus_i_io_mutex_cnt;
#ifndef globus_i_io_mutex_lock
#   define globus_i_io_mutex_lock()         \
    {                           \
        globus_mutex_lock(&globus_i_io_mutex);      \
        globus_i_io_mutex_cnt++;            \
        globus_assert(globus_i_io_mutex_cnt==1);        \
    }
#endif
#ifndef globus_i_io_mutex_unlock
#   define globus_i_io_mutex_unlock()           \
    {                           \
        globus_i_io_mutex_cnt--;            \
        globus_assert(globus_i_io_mutex_cnt==0);        \
        globus_mutex_unlock(&globus_i_io_mutex);    \
    }
#endif


#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#endif

#define NULLSTRING "NULL"

	/* Macro used over and over w/ bad syntax */
#define HANDLE_SYNTAX_ERROR()		\
	{								\
	gahp_printf("E\n");		\
	gahp_sem_up(&print_control);	\
	all_args_free(user_arg);		\
	}

static char *commands_list = 
"COMMANDS "
"GASS_SERVER_INIT " 
"GRAM_CALLBACK_ALLOW "
"GRAM_ERROR_STRING "
"GRAM_JOB_CALLBACK_REGISTER "
"GRAM_JOB_CANCEL "
"GRAM_JOB_REQUEST "
"GRAM_JOB_SIGNAL "
"GRAM_JOB_STATUS "
"GRAM_PING "
"INITIALIZE_FROM_FILE "
"QUIT "
"RESULTS "
"VERSION "
"ASYNC_MODE_ON "
"ASYNC_MODE_OFF "
"RESPONSE_PREFIX "
"REFRESH_PROXY_FROM_FILE "
"GRAM_JOB_REFRESH_PROXY ";

typedef struct gahp_semaphore {
	globus_mutex_t mutex;
} gahp_semaphore;
	

/* GLOBALS */
globus_fifo_t result_fifo;
gahp_semaphore print_control;
gahp_semaphore fifo_control;

static char *VersionString ="$GahpVersion 1.0.6 " __DATE__ " UW\\ Gahp $";
volatile int ResultsPending;
volatile int AsyncResults;
volatile int GlobusActive;
globus_gass_transfer_listener_t gassServerListener;
char *ResponsePrefix = NULL;

/* These are all the async command handlers. */
int handle_gram_job_request(void *);
int handle_gram_job_cancel(void *);
int handle_gram_job_status(void *);
int handle_gram_job_signal(void *);
int handle_gram_job_callback_register(void *);
int handle_gram_job_refresh_proxy(void *);
void * handle_gass_server_init(void *);

/* These are all of the callbacks for non-blocking async commands */
void
callback_gram_job_request(void *arg,
						  globus_gram_protocol_error_t operation_fc,
						  const char *job_contact,
						  globus_gram_protocol_job_state_t job_state,
						  globus_gram_protocol_error_t job_fc);
void
callback_gram_job_signal(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc);
void
callback_gram_job_status(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc);
void
callback_gram_job_cancel(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc);
void
callback_gram_job_callback_register(void *arg,
									globus_gram_protocol_error_t operation_fc,
									const char *job_contact,
									globus_gram_protocol_job_state_t job_state,
									globus_gram_protocol_error_t job_fc);
void
callback_gram_job_refresh_proxy(void *arg,
								globus_gram_protocol_error_t operation_fc,
								const char *job_contact,
								globus_gram_protocol_job_state_t job_state,
								globus_gram_protocol_error_t job_fc);

/* These are all of the sync. command handlers */
int handle_gram_callback_allow(void *);
int handle_gram_error_string(void *);
int handle_async_mode_on(void *);
int handle_async_mode_off(void *);
int handle_response_prefix(void *);
int handle_refresh_proxy_from_file(void *);

/* This is the handler for job status update callbacks from the jobmanager */
void gram_callback_handler(void *callback_arg, char *job_contact, int state,
						   int error);

void main_deactivate_globus();

void gahp_sem_init( gahp_semaphore *, int initial_value);
void gahp_sem_up(  gahp_semaphore *);
void gahp_sem_down( gahp_semaphore * );

void enqueue_results( char *result_line );

/* Escape spaces replaces ' ' with '\ '. It allocates memory, and 
   puts the escaped string in that memory, and returns it. A NULL is
   returned if something (really, just memory allocation) fails. 
   The caller is responsible for freeing the memory returned by this func 
*/ 
char *escape_spaces( const char *input_line );

/* all_args_free frees all the memory passed into a command handler */
void all_args_free( void *);

/* These routines verify that the ascii argument is either a number or an */
/* string - they do not allocate any memory - by default, output_line will */
/* be a pointer pointer to input_line, or NULL if input_line=="NULL" */

int process_string_arg( char *input_line, char **output_line);
int process_int_arg( char *input_line, int *result );

int
gahp_printf(const char *format, ...)
{
	int ret_val;
	va_list ap;
	char buf[10000];

	globus_libc_lock();

	va_start(ap, format);
	vsprintf(buf, format, ap);

	if (ResponsePrefix) {
		ret_val = printf("%s%s",ResponsePrefix,buf);
	} else {
		ret_val = printf("%s",buf);
	}

	fflush(stdout);

	globus_libc_unlock();
	
	return ret_val;
}


void
gahp_sem_init( gahp_semaphore *sem, int initial_value) 
{
	globus_mutex_init(&sem->mutex, NULL);
}

void
gahp_sem_up( gahp_semaphore *sem)
{
	globus_mutex_unlock(&sem->mutex);
}

void
gahp_sem_down( gahp_semaphore *sem)
{
	globus_mutex_lock(&sem->mutex);
}


int 
process_string_arg( char *input_line, char **output)
{
	int i = 0;

	// if it's a NULL pointer, or it points to something that's zero
	// length, give up now.
    if(!input_line){
        return false;
    }
	i = strlen(input_line);
	if(!i) {
		return false;
	}

	// by default, just give back what they gave us.
	*output = input_line;
	// some commands are allowed to pass in "NULL" as their option. If this
	// is the case, set output to point to a NULL string, which we will pass
	// along to the globus routines...

    if(!strcasecmp(input_line, NULLSTRING)) {
    	*output = NULL;  
	}

	return true;
}

int 
process_int_arg( char *input, int *output)
{
	if( input && strlen(input)) {
		*output = atoi(input);
		return true;
	}
	return false;
}

void
enqueue_results( char *output) 
{
	gahp_sem_down(&print_control);
    gahp_sem_down(&fifo_control);
    globus_fifo_enqueue(&result_fifo, (void *)output);

	if(AsyncResults && !ResultsPending) {
		ResultsPending = 1;
		gahp_printf("R\n");
	}
	gahp_sem_up(&fifo_control);
	gahp_sem_up(&print_control);
	return;
}

char *
escape_spaces( const char *input_line) 
{
	int i;
	char *temp;
	char *output_line;

	// first, count up the spaces
	temp = (char *)input_line;
	for(i = 0; *temp != '\0'; temp++) {
		if( *temp == ' ' || *temp == '\r' || *temp =='\n')  i++;
	}

	// get enough space to store it.  	
	output_line = globus_libc_malloc(strlen(input_line) + i + 200);

	// now, blast across it
	temp = (char *)input_line;
	for(i = 0; *temp != '\0'; temp++) {
		if( *temp == ' ') {
			output_line[i] = '\\'; 
			i++;
		}
		if( *temp == '\r' || *temp == '\n') {
			output_line[i] = '\\'; 
			i++;
			*temp = ' ';
		}
		output_line[i] = *temp;
		i++;
	}
	output_line[i] = '\0';
	// the caller is responsible for freeing this memory, not us
	return output_line;	
}

void
all_args_free( void * args)
{
	char **input_line = args;

	/* if we get a NULL pointer, give up now */
	if(!input_line) 
		return;

	/* input_line is a array of char*'s from read_command */
	/* it's a vector of command line arguemts */
	/* The last one will be null, so walk it until we hit the end */
	/* If this was a real library call this would be bad, since we */
	/* trust how we got this memory. */

	while(*input_line) {
		globus_libc_free(*input_line);
		input_line++;
	}
	globus_libc_free(args);
	return;	
}

int
handle_bad_request(void * user_arg)
{
	char **input_line = (char **) user_arg;

	HANDLE_SYNTAX_ERROR();

	return 0;
}


int
handle_gram_job_request(void * user_arg)
{

	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	char *req_id, *resource_contact, *callback_contact, *delegation, *rsl;

	// req id
	if( !process_string_arg(input_line[1], &req_id ) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}
	
	// resource contact string
	if( !process_string_arg(input_line[2], &resource_contact) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}
	// callback contact
	if( !process_string_arg(input_line[3], &callback_contact ) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}
	 
	// use_full_delegation, which is not used...
	if( !process_string_arg(input_line[4], &delegation ) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}
	// rsl
	if( !process_string_arg(input_line[5], &rsl) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	req_id = globus_libc_strdup( req_id );

	gahp_printf("S\n");
	gahp_sem_up(&print_control);
	result = globus_gram_client_register_job_request(resource_contact, rsl, 
							GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL, 
							callback_contact, (globus_gram_client_attr_t)NULL,
							callback_gram_job_request, (void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_request((void *)req_id, result, NULL, 0, 0);
	}

	all_args_free(user_arg);
	return 0;
}

void
callback_gram_job_request(void *arg,
						  globus_gram_protocol_error_t operation_fc,
						  const char *job_contact,
						  globus_gram_protocol_job_state_t job_state,
						  globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d %s", req_id, operation_fc,
						job_contact ? job_contact : NULLSTRING);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

int
handle_gram_job_signal(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	int signal;
	char *req_id, *job_contact,  *signal_string;

	// reqid
	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// job contact
	if( !process_string_arg(input_line[2], &job_contact ) ){
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// signal
	if( !process_int_arg(input_line[3], &signal)  ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if( !process_string_arg(input_line[4], &signal_string) )	{
		HANDLE_SYNTAX_ERROR();
		return 0;
	}
	
	req_id = globus_libc_strdup( req_id );

	gahp_printf("S\n");
	gahp_sem_up(&print_control);
	result = globus_gram_client_register_job_signal(job_contact, signal,
									signal_string,
									(globus_gram_client_attr_t)NULL,
									callback_gram_job_signal, (void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_signal((void *)req_id, result, NULL, 0, 0);
	}

	all_args_free(user_arg);
	return 0;
}

void
callback_gram_job_signal(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d %d %d", req_id, operation_fc,
						job_fc, job_state);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

int
handle_gram_job_status(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	char *req_id, *job_contact;

	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if( !process_string_arg(input_line[2], &job_contact) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	req_id = globus_libc_strdup( req_id );

	gahp_printf("S\n");
	gahp_sem_up(&print_control);
	result = globus_gram_client_register_job_status(job_contact,
									(globus_gram_client_attr_t)NULL,
									callback_gram_job_status, (void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_status((void *)req_id, result, NULL, 0, 0);
	}

	all_args_free(user_arg);
	return 0;
}

void
callback_gram_job_status(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d %d %d", req_id, operation_fc,
						job_fc, job_state);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

int
handle_gram_job_cancel(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	char *req_id, *job_contact;

	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if( !process_string_arg(input_line[2], &job_contact) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	req_id = globus_libc_strdup( req_id );

	gahp_printf("S\n");
	gahp_sem_up(&print_control);
	result = globus_gram_client_register_job_cancel(job_contact,
									(globus_gram_client_attr_t)NULL,
									callback_gram_job_cancel, (void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_cancel((void *)req_id, result, NULL, 0, 0);
	}

	all_args_free(user_arg);
	return 0;
}

void
callback_gram_job_cancel(void *arg,
						 globus_gram_protocol_error_t operation_fc,
						 const char *job_contact,
						 globus_gram_protocol_job_state_t job_state,
						 globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d", req_id, operation_fc);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

void *
handle_gass_server_init(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;
	char *output, *req_id, *job_contact;
	char *gassServerUrl = NULL;
	int port;

	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return NULL;
	}

	if( !process_int_arg(input_line[2], &port) ) {
		HANDLE_SYNTAX_ERROR();
		return NULL;
	}

	gahp_printf("S\n");
	gahp_sem_up(&print_control);


 	result = globus_module_activate( GLOBUS_GASS_SERVER_EZ_MODULE );
    if ( result != GLOBUS_SUCCESS ) {
		/*
        globus_gram_client_callback_disallow( gramCallbackContact );
        globus_module_deactivate( GLOBUS_GRAM_CLIENT_MODULE );
        return false; */
    }

    result = globus_gass_server_ez_init( &gassServerListener, NULL, NULL, NULL,
                                      GLOBUS_GASS_SERVER_EZ_READ_ENABLE |
                                      GLOBUS_GASS_SERVER_EZ_LINE_BUFFER |
                                      GLOBUS_GASS_SERVER_EZ_WRITE_ENABLE,
                                      NULL );


	output = (char *)globus_libc_malloc(500);

	if( result == GLOBUS_SUCCESS) {
    	gassServerUrl = globus_gass_transfer_listener_get_base_url(gassServerListener);
	}

	// TODO: need to account for gassServerUrl being NULL
	globus_libc_sprintf(output, "%s %d %s", req_id, result,
						gassServerUrl ? gassServerUrl : NULLSTRING);

	enqueue_results(output);	

	all_args_free(user_arg);
	return NULL;
}

int
handle_gram_job_callback_register(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	char *req_id, *job_contact, *callback_contact;

	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if( !process_string_arg(input_line[2], &job_contact) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if( !process_string_arg(input_line[3], &callback_contact) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	req_id = globus_libc_strdup( req_id );

	gahp_printf("S\n");
	gahp_sem_up(&print_control);

	result = globus_gram_client_register_job_callback_registration(
									job_contact,
									GLOBUS_GRAM_PROTOCOL_JOB_STATE_ALL,
									callback_contact, 
									(globus_gram_client_attr_t)NULL,
									callback_gram_job_callback_register,
									(void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_callback_register((void *)req_id, result, NULL,
											0, 0);
	}

	all_args_free(user_arg);
	return 0;
}

void
callback_gram_job_callback_register(void *arg,
									globus_gram_protocol_error_t operation_fc,
									const char *job_contact,
									globus_gram_protocol_job_state_t job_state,
									globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d %d %d", req_id, operation_fc,
						job_fc, job_state);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

void *
handle_gram_ping(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;
	char *output, *req_id, *resource_contact;


	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return NULL;
	}

	if( !process_string_arg(input_line[2], &resource_contact ) ){
		HANDLE_SYNTAX_ERROR();
		return NULL;
	}

	gahp_printf("S\n");
	gahp_sem_up(&print_control);
	result = globus_gram_client_ping(resource_contact);

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d", req_id, result);

	enqueue_results(output);	

	all_args_free(user_arg);
	return NULL;

}

int
handle_gram_job_refresh_proxy(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;

	// what the arguments get processed into
	char *req_id, *job_contact;

	// reqid
	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// job contact
	if( !process_string_arg( input_line[2], &job_contact ) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	req_id = globus_libc_strdup( req_id );

	gahp_printf( "S\n" );
	gahp_sem_up( &print_control );
	result = globus_gram_client_register_job_refresh_credentials( job_contact,
									GSS_C_NO_CREDENTIAL,
									(globus_gram_client_attr_t)NULL,
									callback_gram_job_refresh_proxy,
									(void *)req_id);

	if (result != GLOBUS_SUCCESS) {
		callback_gram_job_refresh_proxy((void *)req_id, result, NULL, 0, 0);
	}

	all_args_free( user_arg );
	return 0;
}

void
callback_gram_job_refresh_proxy(void *arg,
								globus_gram_protocol_error_t operation_fc,
								const char *job_contact,
								globus_gram_protocol_job_state_t job_state,
								globus_gram_protocol_error_t job_fc)
{
	char *req_id;
	char *output;

	req_id = (char *)arg;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %d", req_id, operation_fc);

	enqueue_results(output);	

	globus_libc_free( req_id );
	return;
}

int 
handle_gram_callback_allow(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result, port;
	char *req_id, *callback_contact = NULL, *error_string = NULL;
	char *saved_req_id = NULL;

	if( !process_string_arg(input_line[1], &req_id) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// what port should we try and use by default...
	if( !process_int_arg(input_line[2], &port) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// We need to stash a copy of of the req_id so Globus can give it to us
	// in our callback

	saved_req_id = globus_libc_malloc(strlen(req_id) + 1);
	strcpy(saved_req_id, req_id);
	result = globus_gram_client_callback_allow(gram_callback_handler,
											   saved_req_id,
											   &callback_contact );

	if( result != GLOBUS_SUCCESS) {
		error_string = escape_spaces(globus_gram_client_error_string(result));
		gahp_printf("F %d %s\n",  result, error_string);
		globus_libc_free(error_string);
	} else {
		gahp_printf("S %s\n", callback_contact);
		globus_libc_free(callback_contact);
	}
	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;
}

void
gram_callback_handler(void *callback_arg, char *job_contact, int state,
					  int error)
{
	char *output;

	output = (char *)globus_libc_malloc(500);

	globus_libc_sprintf(output, "%s %s %d %d", (char *)callback_arg,
						job_contact, state, error);

	enqueue_results(output);
}

int
handle_commands(void * user_arg)
{
	char **input_line = (char **) user_arg;
	gahp_printf("S %s\n", commands_list);
	gahp_sem_up(&print_control);
	all_args_free(user_arg);
	return 0;
}

int 
handle_results(void * user_arg)
{
	char **input_line = (char **) user_arg;
	char *output;
	int count, i;

	gahp_sem_down(&fifo_control);
	count = globus_fifo_size(&result_fifo);

	gahp_printf("S %d\n", count);
	if(count > 0) {
		for(i = 0; i < count; i++) {
			output = globus_fifo_dequeue(&result_fifo);
			gahp_printf("%s\n", output);
			globus_libc_free(output);
		}
	}
	ResultsPending = 0;
	gahp_sem_up(&fifo_control);
	gahp_sem_up(&print_control);

	all_args_free(user_arg);

	return 0;
}

int 
handle_version(void * user_arg)
{
	char **input_line = (char **) user_arg;
	gahp_printf("S %s\n", VersionString);

	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;
}

int
handle_gram_error_string( void * user_arg)
{
	char **input_line = (char **) user_arg;
	char *output;
	const char *error_string = NULL;
	int error_code;

    if( !process_int_arg(input_line[1], &error_code) ) {
		HANDLE_SYNTAX_ERROR();
        return 0;
    }

	error_string = globus_gram_client_error_string(error_code);

	if( error_string )	{
		output = escape_spaces( error_string );
		gahp_printf("S %s\n", output); 
		globus_libc_free(output);
	} else{
		gahp_printf("F Unknown\\ Error\n"); 
	}

	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;

}

int
handle_async_mode_on( void * user_arg)
{
	char **input_line = (char **) user_arg;

	gahp_sem_down(&fifo_control);
	AsyncResults = 1;
	ResultsPending = 0;
	gahp_printf("S\n"); 

	gahp_sem_up(&fifo_control);
	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;

}

int
handle_async_mode_off( void * user_arg)
{
	char **input_line = (char **) user_arg;

	gahp_sem_down(&fifo_control);
	AsyncResults = 0;
	ResultsPending = 0;
	gahp_printf("S\n"); 
	gahp_sem_up(&fifo_control);
	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;

}

int
handle_refresh_proxy_from_file(void * user_arg)
{
	char **input_line = (char **) user_arg;
	char *file_name, *environ_variable;
	gss_cred_id_t credential_handle = GSS_C_NO_CREDENTIAL;
	OM_uint32 major_status; 
	OM_uint32 minor_status; 

	int result;

	file_name = NULL;

	if(!process_string_arg(input_line[1], &file_name) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	// if setenv copies it's second argument, this leaks memory
	if(file_name) {
		environ_variable = globus_libc_malloc(strlen(file_name) + 1);
		strcpy(environ_variable, file_name); 
		globus_libc_setenv("X509_USER_PROXY", environ_variable, 1);	
	}

	// this is a macro, not a function - hence the lack of a semicolon
	globus_i_io_mutex_lock()
	major_status = globus_gss_assist_acquire_cred(&minor_status,
					GSS_C_INITIATE, /* or GSS_C_ACCEPT */
					&credential_handle); 
	globus_i_io_mutex_unlock()

	if (major_status != GSS_S_COMPLETE) 
	{ 
			gahp_printf("F Error\\ Codes:\\ %d\\ %d\n", major_status,
								minor_status);	
			gahp_sem_up(&print_control);
			all_args_free(user_arg);
			return 0;
	} 

	globus_gram_client_set_credentials(credential_handle);

	gahp_printf("S\n");

	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;
}


int 
handle_quit(void * user_arg)
{
	char **input_line = (char **) user_arg;
	gahp_printf("S\n");
	main_deactivate_globus();
	_exit(0);

// never reached
	gahp_sem_up(&print_control);
	return 0;
}

int
handle_initialize_from_file(void * user_arg)
{
	char **input_line = (char **) user_arg;
	char *file_name;
	int result;

	file_name = NULL;

	if(!process_string_arg(input_line[1], &file_name) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	if(file_name) {
		globus_libc_setenv("X509_USER_PROXY", file_name, 1);	
	}

	if ( (result=main_activate_globus()) != GLOBUS_SUCCESS ) {
		gahp_printf("F %d Failed\\ to\\ activate\\ Globus modules\n",
							result);

		gahp_sem_up(&print_control);
		all_args_free(user_arg);
		return 0;
	}

	GlobusActive = true;
	gahp_printf("S\n");

	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;
}

int
handle_response_prefix(void * user_arg)
{
	char **input_line = (char **) user_arg;
	int result;
	char *prefix;

	if(!process_string_arg(input_line[1], & prefix ) ) {
		HANDLE_SYNTAX_ERROR();
		return 0;
	}

	gahp_printf("S\n");

		/* Now set global ResponsePrefix _before_ releasing the 
		 * print_control mutex. */
	if (ResponsePrefix) {
		free(ResponsePrefix);
		ResponsePrefix = NULL;
	}
	if ( prefix ) {
		ResponsePrefix = globus_libc_strdup(prefix);
	}


	gahp_sem_up(&print_control);

	all_args_free(user_arg);
	return 0;
}

int
main_activate_globus()
{
	int err;


	err = globus_module_activate( GLOBUS_GRAM_CLIENT_MODULE );
	if ( err != GLOBUS_SUCCESS ) {
		globus_module_deactivate( GLOBUS_GRAM_CLIENT_MODULE );
		return err;
	}

	return GLOBUS_SUCCESS;
}


void
main_deactivate_globus()
{
		// So sometimes calling globus_module_deactivate_all() hangs,
		// usually when trying to shutdown GASS.  :(.  So for now
		// we just comment it out, and let the operating system try
		// to reclaim all the resources.
	/* globus_module_deactivate_all(); */

	return;
}


char **
read_command(int stdin_fd)
{
	static char* buf = NULL;
	char** command_argv;
	int ibuf = 0;
	int iargv = 0;
	int result = 0;

	if ( buf == NULL ) {
		buf = globus_libc_malloc(1024 * 500);
	}

	/* Read a command from stdin.  We use the read() system call for this,
	 * since the POSIX threads standard requires that read just block
	 * the thread and not the entire process.  We do _not_ use 
	 * globus_libc_read() here because that holds onto a libc mutex 
	 * until read returns --- this would halt our other threads.
	 */
	
	command_argv = globus_libc_calloc(500, sizeof(char*));

	ibuf = 0;
	iargv = 0;

	for (;;) {
		result = read(stdin_fd, &(buf[ibuf]), 1 );

		/* Check return value from read() */
		if ( result < 0 ) {		/* Error reading */
			if ( errno == EINTR ||
				errno == EAGAIN ||
				errno == EWOULDBLOCK )
			{
				continue;
			}
			_exit(2);	/* some sort of fatal error */
		}
		if ( result == 0 ) {	/* End of File */
			/* Exit right here and now.  Don't try to deactivate globus
			 * modules first...  it appears this may cause a needless segfault 
			 */
#ifndef WIN32
			_exit(SIGPIPE);
#else
			_exit(3);
#endif
		}

		/* Check if charcater read was whitespace */
		if ( buf[ibuf]==' ' || buf[ibuf]=='\t' || buf[ibuf]=='\r' ) {
			/* Ignore leading whitespace */
			if ( ibuf == 0 ) {	
				continue;
			}
			/* Handle Transparency: if char is '\' followed by a space,
			 * it should be considered a space and not as a seperator
			 * between arguments. */
			if ( buf[ibuf]==' ' && buf[ibuf-1]=='\\' ) {
				buf[ibuf-1] = ' ';
				continue;
			}
			/* Trailing whitespace delimits a parameter to copy into argv */
			buf[ibuf] = '\0';
			command_argv[iargv] = globus_libc_malloc(ibuf + 5);
			strcpy(command_argv[iargv],buf);
			ibuf = 0;
			iargv++;
			continue;
		}

		/* If character was a newline, copy into argv and return */
		if ( buf[ibuf]=='\n' ) { 
			buf[ibuf] = '\0';
			command_argv[iargv] = globus_libc_malloc(ibuf + 5);
			strcpy(command_argv[iargv],buf);
			return command_argv;
		}

		/* Character read was just a regular one.. increment index
		 * and loop back up to read the next character */
		ibuf++;

	}	/* end of infinite for loop */
}


/* Two macros, either handle the event asynchornously (by spawning a 
 * a new thread) or syncrhonously by just doing it. The loop at the
 * bottom looks at "result" to decide how it worked - result will either
 * we didn't even try and start a thread. 
 */

#define HANDLE_ASYNC( x ) \
	if (strcasecmp(#x,input_line[0]) == 0) { \
		if(GlobusActive) { \
			result=globus_thread_create(&ignored_threadid, \
						NULL,handle_##x,(void *)input_line);\
			} \
		else {  result=-1; } \
		}

#define HANDLE_SYNC( x ) \
	if (strcasecmp(#x,input_line[0]) == 0) \
		result=handle_##x(input_line);

void
service_commands()
{
	char **input_line;
	int result;
	globus_thread_t ignored_threadid;

	for(;;) {

		input_line = read_command(STDIN_FILENO);

		if ( input_line == NULL ) {
			return;
		}

		if ( input_line[0] == NULL ) {
			return;
		}

		// Each command that we run will return the semaphore to our control,
		// which will prevent it from printing things out in our stream
		gahp_sem_down(&print_control);

		HANDLE_SYNC( initialize_from_file ) else
		HANDLE_SYNC( commands ) else
		HANDLE_SYNC( version ) else
		HANDLE_SYNC( quit ) else
		HANDLE_SYNC( results ) else
		HANDLE_SYNC( async_mode_on ) else
		HANDLE_SYNC( async_mode_off ) else
		HANDLE_SYNC( response_prefix ) else
		HANDLE_SYNC( gram_error_string ) else
		HANDLE_SYNC( refresh_proxy_from_file ) else
		HANDLE_SYNC( gram_callback_allow) else
		HANDLE_SYNC( gram_job_request ) else
		HANDLE_SYNC( gram_job_cancel ) else
		HANDLE_SYNC( gram_job_status ) else
		HANDLE_SYNC( gram_job_callback_register ) else
		HANDLE_ASYNC( gram_ping ) else
		HANDLE_ASYNC( gass_server_init ) else 
		HANDLE_SYNC( gram_job_signal ) else
		HANDLE_SYNC( gram_job_refresh_proxy ) else
		{
			handle_bad_request(input_line);
			result = 0;
		}

		if ( result != 0 ) {
			/* could not create a thread to handle the command... now what??
		 	* for now, flag it as a bad request.  Sigh.
		 	*/
			handle_bad_request(input_line);
		}


	}	/* end of infinite loop */

}


void
quit_on_signal(int sig)
{
	/* Posix says exit() is not signal safe, so call _exit() */
	_exit(sig);
}


void
quit_on_signal_with_core(int sig)
{
#ifndef WIN32
	sigset_t sigSet;
	struct sigaction act;

	/* A signal has been raised which should drop a core file.  The problem
	 * is that the core file from a multi-threaded process is often useless
	 * to try and look at with gdb on certain platforms.
	 * So, what we do here is first fork(), and then blow up with a core file.
	 * Why?  Because POSIX says that when a thread calls fork(), the child
	 * process will be created with just _one_ thread, and that one thread
	 * will contain the context of the thread in the parent which called fork().
	 * Thus we will end up with a core file with the one thread context which
	 * we are interested in, and also a core file which gdb can deal with.
	 * Ain't we smart?  --- Todd Tannenbaum <tannenba@cs.wisc.edu>
	 */
	int pid;
	pid = fork();
	if ( pid == -1 ) {
		_exit(66);
	}
	if ( pid ) {
		/* PARENT */
		_exit(sig);		/* call _exit because exit() is not signal safe */
	} else {
		/* CHILD */
		/* Ok, our parent is in the process of exiting.  Our job here in
		 * the child is to blow up with the critical signal, and as a result
		 * leave behind a decent core file. 
		 * 
		 * Currently, the critical signal is blocked because we are in the 
		 * handler for that signal, and this is what sigaction does by default.
		 * What we want to do is 
		 * 		a) reset the handler for this signal to the
		 * 			system default handler (which will drop core), and then 
		 * 		b) send ourselves the signal, and then 
		 * 		c) unblock the signal.
		 */
		sigemptyset(&act.sa_mask);	
		act.sa_flags = 0;
		act.sa_handler = SIG_DFL;
		sigaction(sig,&act,0);		/* reset handler */
		kill(getpid(),sig);			/* send ourselves the signal - now it is pending */
		sigemptyset(&sigSet);
		sigaddset(&sigSet,sig);
		sigprocmask(SIG_UNBLOCK,&sigSet,0);	/* unblock the signal.... BOOOM! */
		/* Should not make it here --- but exit just in case */
		_exit(sig);
	}
#endif
}


main(int argc, char **argv)
{
	int result, err;
#ifndef WIN32
	sigset_t sigSet;
	struct sigaction act;
#endif
	ResultsPending = 0;
	GlobusActive = 0;
	AsyncResults = 0;

#ifndef WIN32
	/* Add the signals we want unblocked into sigSet */
	sigemptyset( &sigSet );
	sigaddset(&sigSet,SIGTERM);
	sigaddset(&sigSet,SIGQUIT);
	sigaddset(&sigSet,SIGPIPE);

	/* Set signal handlers */
	sigemptyset(&act.sa_mask);	/* do not block anything in handler */
	act.sa_flags = 0;
	
	/* Signals which should cause us to exit with status = signum */
	act.sa_handler = quit_on_signal;
	sigaction(SIGTERM,&act,0);
	sigaction(SIGQUIT,&act,0);
	sigaction(SIGPIPE,&act,0);

#ifdef FORK_FOR_CORE
	/* Signals which should cause us to exit with a usefull core file */
	act.sa_handler = quit_on_signal_with_core;
	sigaddset(&sigSet,SIGILL);
	sigaction(SIGILL,&act,0);
	sigaddset(&sigSet,SIGABRT);
	sigaction(SIGABRT,&act,0);
	sigaddset(&sigSet,SIGFPE);
	sigaction(SIGFPE,&act,0);
	sigaddset(&sigSet,SIGSEGV);
	sigaction(SIGSEGV,&act,0); 
#	ifdef SIGBUS
	sigaddset(&sigSet,SIGBUS);
	sigaction(SIGBUS,&act,0);
#	endif
#endif

	/* Unblock signals in our set */
	sigprocmask( SIG_UNBLOCK, &sigSet, NULL );

#endif

	/* Parse command line args */

	/* Activate Globus modules we intend to use */
    err = globus_module_activate( GLOBUS_COMMON_MODULE );
    if ( err != GLOBUS_SUCCESS ) {
		printf("Unable to active Globus!\n");
    }


	if ( (result=globus_fifo_init(&result_fifo)) != GLOBUS_SUCCESS ) {
		printf("ERROR %d Failed to activate Globus fifo\n",result);
		_exit(1);
	}

	gahp_sem_init(&print_control, 1);
	gahp_sem_init(&fifo_control, 1);

	gahp_printf("%s\n", VersionString);

	/* At this point and forward, all code better be thread happy */

	/* Service commands from stdin... this will not return unless we are told
	 * to exit. */
	service_commands();

	main_deactivate_globus();
	_exit(0);
}
