FROM ubuntu:xenial

RUN apt-get update

RUN apt-get -y install make automake libtool pkg-config libaio-dev git

# For MySQL support
RUN apt-get -y install libmysqlclient-dev libssl-dev

# For PostgreSQL support
RUN apt-get -y install libpq-dev

RUN git clone https://github.com/akopytov/sysbench.git sysbench

WORKDIR sysbench
RUN ./autogen.sh
RUN ./configure --with-mysql --with-pgsql
RUN make -j
RUN make install

WORKDIR /root
RUN rm -rf sysbench

ENTRYPOINT sysbench
