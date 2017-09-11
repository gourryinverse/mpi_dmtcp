git submodule update --init --recursive
cd dmtcp
./configure
make
make check
make install
sudo apt-get install mpich -y
