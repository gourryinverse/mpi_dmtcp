./clean.sh
mpirun dmtcp_launch --with-plugin ./plugin/mpi_tutorials/libdmtcp_mpi_tutorials.so ./mpitutorial/tutorials/mpi-hello-world/code/mpi_hello_world
mpirun ./dmtcp_restart_script.sh
