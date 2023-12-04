/************************************************************************
* 
* This can be compiled by using g++:
* 
* g++ -o metrics_test_server metrics_test_server.cc
* 
* Then, say you want to bind to multiple ports:
* 
* ./metrics_test_server 4001 4002 4003
* 
* This will run the server, which will bind to ports 4001, 4002, and 4003 on the
* loopback interface.  Then you can run the metrics reporter (this is under bash):
* 
* PEGASUS_USER_METRICS_SERVER='http://localhost:4001,http://localhost:4002' \
* ./libexec/condor_dagman_metrics_reporter -f ~/metrics-test.txt \
* -u 'http://localhost:4003'
* 
* Here is the metrics-test.txt file:
* {
*     "client": "pegasus-plan",
*     "version": "4.2.2",
*     "type": "metrics",
*     "wf_uuid": "efc309d4-0ca8-4492-be02-3bf931451234",
*     "root_wf_uuid": "efc309d4-0ca8-4492-be02-3bf931451234",
*     "start_time": 1371570219.048,
*     "end_time": 1371571617.634,
*     "duration": 1398.586,
*     "exitcode": 1,
*     "dagman_version": "8.0.0",
*     "dagman_id": "manta.cs.wisc.edu:1234:99",
*     "parent_dagman_id": "",
*     "rescue_dag_number": 0,
*     "jobs": 10,
*     "jobs_failed": 1,
*     "jobs_succeeded": 5,
*     "dag_jobs": 2,
*     "dag_jobs_failed": 0,
*     "dag_jobs_succeeded": 2,
*     "total_jobs": 12,
*     "total_jobs_run": 8,
*     "total_job_time": 1968.000,
*     "dag_status": 4
* }
* Here is the output:
* Executing: "./libexec/condor_dagman_metrics_reporter" "-f" "/u/n/w/nwp/metrics-test.txt" "-u" "http://localhost:4003"
* 
* Will attempt to contact the following metrics servers:
* 	http://localhost:4001
* 	http://localhost:4002
* 	http://localhost:4003
* 
* Data to send: <{"client": "pegasus-plan","version": "4.2.2","type": "metrics","wf_uuid": "efc309d4-0ca8-4492-be02-3bf931451234","root_wf_uuid": "efc309d4-0ca8-4492-be02-3bf931451234","start_time": 1371570219.048,"end_time": 1371571617.634,"duration": 1398.586,"exitcode": 1,"dagman_version": "8.0.0","dagman_id": "manta.cs.wisc.edu:1234:99","parent_dagman_id": "","rescue_dag_number": 0,"jobs": 10,"jobs_failed": 1,"jobs_succeeded": 5,"dag_jobs": 2,"dag_jobs_failed": 0,"dag_jobs_succeeded": 2,"total_jobs": 12,"total_jobs_run": 8,"total_job_time": 1968.000,"dag_status": 4}>
* 
* Setting the POST option... Success!
* Setting the data to send... Success!
* Set the content length... Success!
* Telling server we are sending json... Success!
* Initializing the error buffer... Success!
* Successfully sent data to server http://localhost:4001
* Successfully sent data to server http://localhost:4002
* Successfully sent data to server http://localhost:4003
************************************************************************/

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <signal.h>
#include <sstream>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
int main(int argc,char* argv[])
{
	if(argc < 2) return 1;
	setsid();
	pid_t pid = fork();
	if(pid > 0) _exit(0);
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP|SA_NOCLDWAIT;
	sigaction(SIGCHLD,&sa,0);
	std::vector<int> sockets;
	for(int ii=1;ii<argc;++ii) {
		int sck = socket(AF_INET,SOCK_STREAM,0);
		if(sck<0) {
			continue;
		}
		char* end = 0;
		int port = std::strtol(argv[ii],&end,10);
		if( end ) {
			if( *end != '\0' ) {
				close(sck);
				continue;
			}
		}
		std::cerr << "Binding to port " << port << std::endl;
		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port);
		sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if(bind(sck,(struct sockaddr*)&sin,sizeof sin)) {
			close(sck);
			continue;
		}
		if(listen(sck,5)) {
			close(sck);
			std::cerr << "Could not listen on socket";
			continue;
		}
		sockets.push_back(sck);
	}
	if(sockets.empty()) {
		std::cerr << "No valid ports set!\n";
		return 1;
	}
	std::sort(sockets.begin(),sockets.end());
	for(;;) {
		fd_set fds;
		FD_ZERO(&fds);
		int maxfd = 0;
		for(std::vector<int>::iterator pp = sockets.begin(); pp != sockets.end();
				++pp) {
			FD_SET(*pp,&fds);
			if(*pp>maxfd) maxfd = *pp + 1;
		}
		if(select(maxfd,&fds,0,0,0) <= 0) {
			std::cerr << "select terminated.  Exiting the program" << std::endl;
			break;
		}
		for(std::vector<int>::iterator pp = sockets.begin(); pp != sockets.end();
				++pp) {
			if(FD_ISSET(*pp,&fds)) {
				int cfd = accept(*pp,0,0);
				if(cfd < 0) continue;
				pid = fork();
				if(pid > 0) {
					close(cfd);
					continue;
				}
				if(pid == 0) {
					for(std::vector<int>::iterator qq = sockets.begin();
							qq != sockets.end();
							++qq) {
						close(*qq);
					}
					pid = getpid();
					std::stringstream ss;
					ss << "server." << pid << ".rcv";
					std::ofstream out(ss.str().c_str(),std::ios_base::out|
						std::ios_base::ate|std::ios_base::app);
					char buf[1024];
					int nread = read(cfd,&buf[0],sizeof buf);
					out.write(&buf[0],nread);
					out << "\n" << std::endl;	
					std::stringstream toclient;
					toclient << "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n";
					write(cfd,toclient.str().data(),toclient.str().size());
					shutdown(cfd,SHUT_RDWR);
					close(cfd);
					_exit(0);
				}
			}
		}
	}
	return 0;
}

