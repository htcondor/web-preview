# Dockerfile to checkout and build HTCondor source code

# Start from Centos7
FROM centos

# Create a unix user to run build as
RUN adduser build

# Install prereqs
RUN yum -y install autotools git cmake make gcc gcc-c++ gcc-fortran pam-devel libcurl libcurl-devel boost-devel pcre-devel libxml2-devel libuuid-devel glibc-static sqlite-devel patch python-devel bison flex openssl-devel nss-devel perl-Data-Dumper

# Checkout and build condor as unix user "build"
RUN su - build /bin/bash -c 'git clone https://github.com/htcondor/htcondor.git && cd htcondor && ./configure_uw -DWITH_CREAM:BOOL=false -DWITH_GLOBUS:BOOL=false -DWITH_BLAHP:BOOL=false -DCLIPPED:BOOL=true -DWITH_BOINC:BOOL=false && make'

