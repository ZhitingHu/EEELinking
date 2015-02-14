# .bashrc

# Source global definitions
if [ -f /etc/bashrc ]; then
	. /etc/bashrc
fi

# User specific aliases and functions
alias lab='ssh root@localhost -p 19999'
alias w='cd /home/yuntiand/public/apps/sparsecoding'

module load matlab-8.1 gcc-4.7.3 boost-1.50 python27
export PATH=/opt/autoconf-2.68/bin/:$PATH
export PREFIX=/home/yuntiand/prefix
export LD_LIBRARY_PATH=/home/yuntiand/caffe_third_party/lib:/home/yuntiand/caffe_third_party/opencv-2.4.9/lib:$LD_LIBRARY_PATH
export PATH=$PREFIX/bin:/opt/autoconf-2.68/bin/:/home/yuntiand/caffe_third_party/bin:$PATH
