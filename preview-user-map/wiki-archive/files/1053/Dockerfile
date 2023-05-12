FROM ubuntu:bionic

# Ensure that gethostbyaddr() returns our hostname
RUN sed -i -e 's/^hosts:.*/& myhostname/' /etc/nsswitch.conf

# Add users that might be used in CHTC
# The HTCondor that runs inside the container needs to have the user defined
RUN useradd --uid  1337 --shell /bin/bash --create-home condor

# Add tools to add the HTCondor repository
RUN apt update && apt install -y \
    apt-transport-https \
    gnupg \
    wget \
    && apt clean

# Add HTCondor repository
#RUN wget -qO - https://research.cs.wisc.edu/htcondor/debian/HTCondor-Release.gpg.key | apt-key add -
#RUN echo "deb [arch=amd64] http://research.cs.wisc.edu/htcondor/ubuntu/8.8/bionic bionic contrib" > /etc/apt/sources.list.d/htcondor.list

# Need HTCondor to build HTCondor (BatLab style)
#RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y \
    #htcondor \
    #libnss-myhostname \
    #openssh-server \
    #&& apt clean

# Tools needed for building
RUN apt update && apt install -y \
    build-essential \
    devscripts \
    && apt clean

# HTCondor build dependencies
RUN apt update && DEBIAN_FRONTEND=noninteractive apt install -y \
    autoconf \
    automake \
    bison \
    chrpath \
    cmake \
    debhelper \
    default-jdk \
    dh-autoreconf \
    dh-python \
    fakeroot \
    flex \
    help2man \
    libboost-filesystem-dev \
    libboost-program-options-dev \
    libboost-python-dev \
    libboost-regex-dev \
    libboost-system-dev \
    libboost-test-dev \
    libboost-thread-dev \
    libc-ares-dev \
    libcgroup-dev \
    libcurl4-openssl-dev \
    libdistro-info-perl \
    libexpat1-dev \
    libglobus-ftp-client-dev \
    libglobus-gass-server-ez-dev \
    libglobus-gram-client-dev \
    libglobus-gram-protocol-dev \
    libglobus-gss-assist-dev \
    libkrb5-dev \
    libldap2-dev \
    libmunge-dev \
    libpam0g-dev \
    libpcre3-dev \
    libpq-dev \
    libsqlite3-dev \
    libtool \
    libtool-bin \
    libvirt-dev \
    libx11-dev \
    libxml2-dev \
    libxss-dev \
    pkg-config \
    po-debconf \
    python3-all-dev \
    python3-setuptools \
    python-dev \
    python-setuptools \
    python-sphinx \
    python-sphinx-rtd-theme \
    uuid-dev \
    zlib1g-dev \
    && apt clean

RUN apt update && apt install -y \
    python3-pip \
    && apt clean

RUN pip3 install pytest pytest-httpserver



# Checkout and build condor as unix user "condor"
RUN su - condor /bin/bash -c 'git clone https://github.com/htcondor/htcondor.git && mkdir build && cd build && ../htcondor/configure_uw -DWITH_CREAM:BOOL=false -DWITH_GLOBUS:BOOL=true -DWITH_VOMS:BOOL=false -DWITH_BLAHP:BOOL=false -DWITH_BOINC:BOOL=false -D_DEBUG:BOOL=true ../htcondor && make install'

# setup condor dirs
RUN su - condor /bin/bash -c 'cd && mkdir local && cd local && mkdir log spool execute config'

# setup certs
RUN su - condor /bin/bash -c 'cd && cd local && tar zxf /home/condor/htcondor/src/condor_tests/x_lib_auth-gsi_keys.tgz'

# setup main config
RUN su - condor /bin/bash -c 'cp /home/condor/build/release_dir/etc/examples/condor_config /home/condor'
RUN su - condor /bin/bash -c 'echo "RELEASE_DIR = /home/condor/build/release_dir" >> /home/condor/condor_config'
RUN su - condor /bin/bash -c 'echo "LOCAL_DIR = /home/condor/local" >> /home/condor/condor_config'

# setup local config
RUN su - condor /bin/bash -c 'echo "# basic config for gsi-enabled daemons" > /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "DAEMON_LIST = MASTER COLLECTOR NEGOTIATOR STARTD SCHEDD" >> /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "CONDOR_HOST = 127.0.0.1" >> /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "GSI_DAEMON_DIRECTORY = /home/condor/local/gsi_keys" >> /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "GSI_DAEMON_NAME = *" >> /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "TOOL_DEBUG = D_SECURITY:2 D_NETWORK:2" >> /home/condor/local/condor_config.local'
RUN su - condor /bin/bash -c 'echo "SCHEDD_DEBUG = D_SECURITY:2 D_NETWORK:2" >> /home/condor/local/condor_config.local'

# setup gsi testing config
RUN su - condor /bin/bash -c 'echo "# gsi testing config" > /home/condor/condor_config.gsi'
RUN su - condor /bin/bash -c 'echo "SEC_CLIENT_AUTHENTICATION = REQUIRED" >> /home/condor/condor_config.gsi'
RUN su - condor /bin/bash -c 'echo "SEC_CLIENT_AUTHENTICATION_METHODS = GSI" >> /home/condor/condor_config.gsi'

# setup user .globus dir to use proxy
RUN su - condor /bin/bash -c 'cd && mkdir .globus && cd .globus && ln -s /home/condor/local/gsi_keys/hostproxycert hostcert.pem && ln -s /home/condor/local/gsi_keys/hostproxycert hostkey.pem'

# setup user path and condor_config
RUN su - condor /bin/bash -c 'echo "export CONDOR_CONFIG=/home/condor/condor_config" >> /home/condor/.profile'
RUN su - condor /bin/bash -c 'echo "export PATH=/home/condor/build/release_dir/bin:/home/condor/build/release_dir/sbin:$PATH" >> /home/condor/.profile'

# supply repro command
RUN su - condor /bin/bash -c 'echo "condor_ping -config /home/condor/condor_config.gsi -debug DAEMON" > /home/condor/run_this'

