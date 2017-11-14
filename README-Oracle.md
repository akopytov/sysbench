**WARNING: Oracle support is unmaintained as of sysbench 1.0. You may
want to
try [sysbench 0.5](https://github.com/akopytov/sysbench/tree/0.5)
instead. The corresponding code and instructions below are still in the
source tree in case somebody wants to update them. Patches are always
welcome! **

--------------------------------------------------------------
Oracle Build steps
--------------------------------------------------------------

Using Ubuntu 14.04 - intructions dated for 21/09/2016 (Was built on AWS
in an r3.xlarge These actions were done against 0.5 checkout)

* Setup Oracle Instant Client -
https://help.ubuntu.com/community/Oracle%20Instant%20Client download
from
http://www.oracle.com/technetwork/database/features/instant-client/index-097480.html.

The following RPM's and upload them to the server:
- oracle-instantclient12.1-basic-12.1.0.2.0-1.x86_64.rpm
- oracle-instantclient12.1-devel-12.1.0.2.0-1.x86_64.rpm

```
alien -i oracle-instantclient12.1-basic-12.1.0.2.0-1.x86_64.rpm
alien -i oracle-instantclient12.1-devel-12.1.0.2.0-1.x86_64.rpm
```

* Install Cuda - http://www.r-tutor.com/gpu-computing/cuda-installation/cuda7.5-ubuntu

```
wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1404/x86_64/cuda-repo-ubuntu1404_7.5-18_amd64.deb
sudo dpkg -i cuda-repo-ubuntu1404_7.5-18_amd64.deb 
sudo apt-get update
sudo apt-get install cuda
export CUDA_HOME=/usr/local/cuda-7.5 
export LD_LIBRARY_PATH=${CUDA_HOME}/lib64 
 
PATH=${CUDA_HOME}/bin:${PATH}
export PATH
echo "/usr/lib/oracle/12.1/client64/lib" > /etc/ld.so.conf.d/oracle-client-12.1.conf
ldconfig
```

* Build sysbench
Use the following `configure` option to build with Oracle support:
```
./configure --with-oracle="/usr/lib/oracle/12.1/client64"
```

Run the following commands to allow sysbench use the full number of cores:
```
sudo sh -c 'for x in /sys/class/net/eth0/queues/rx-*; do echo ffffffff> $x/rps_cpus; done'
sudo sh -c "echo 32768 > /proc/sys/net/core/rps_sock_flow_entries"
sudo sh -c "echo 4096 > /sys/class/net/eth0/queues/rx-0/rps_flow_cnt"
```
