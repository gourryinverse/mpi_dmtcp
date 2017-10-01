git submodule update --init --recursive
sudo apt-get update 
sudo apt-get upgrade -y

# get deps for, configure, build, and install mpich
sudo apt-get install autoconf automake libtool gfortran -y
cd mpich 
./autogen.sh
./configure
make
sudo make install

# get deps for, configure, build, and install dmtcp
cd ..
cd dmtcp
./configure
make
make check
sudo make install
