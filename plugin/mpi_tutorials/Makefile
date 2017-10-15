# To demonstrate, do:  make check    [Checkpoints every 5 seconds]

MPICC = mpicc

# The name will be the same as the current directory name.
NAME=${shell basename $$PWD}

# By default, your resulting plugin library will have this name.
LIBNAME=libdmtcp_${NAME}

# As you add new files to your plugin library, add the object file names here.
LIBOBJS = mpi_tutorials.o

# Modify if your DMTCP_ROOT is located elsewhere.
ifndef DMTCP_ROOT
  DMTCP_ROOT=../../dmtcp
endif
DMTCP_INCLUDE=${DMTCP_ROOT}/include

override CFLAGS += -fPIC -I${DMTCP_INCLUDE}
override CXXFLAGS += -fPIC -I${DMTCP_INCLUDE}
LINK = ${CC}

DEMO_PORT=7781

default: ${LIBNAME}.so mpi_proxy

${DMTCP_ROOT}/test/dmtcp1: ${DMTCP_ROOT}/test
	cd ${DMTCP_ROOT}/test; make dmtcp1
check: ${LIBNAME}.so ${DMTCP_ROOT}/test/dmtcp1
	# Kill an old coordinator on this port if present, just in case.
	@ ${DMTCP_ROOT}/bin/dmtcp_command --quit --quiet \
	  --coord-port ${DEMO_PORT} 2>/dev/null || true
	${DMTCP_ROOT}/bin/dmtcp_launch --coord-port ${DEMO_PORT} --interval 5 \
	  --with-plugin $$PWD/${LIBNAME}.so ${DMTCP_ROOT}/test/dmtcp1

mpi_proxy.o: mpi_proxy.c
	${MPICC} -I${DMTCP_INCLUDE} -c -g3 -O0 -o $@ $<

mpi_proxy: mpi_proxy.o
	${MPICC} -g3 -O0 -o $@ $<

${LIBNAME}.so: ${LIBOBJS}
	${LINK} -shared -fPIC -g3 -O0 -o $@ $^

.c.o:
	${CC} ${CFLAGS} -g3 -O0 -c -o $@ $<

.cpp.o:
	${CXX} ${CXXFLAGS} -c -o $@ $<

tidy:
	rm -f *~ .*.swp dmtcp_restart_script*.sh ckpt_*.dmtcp

clean: tidy
	rm -f ${LIBOBJS} ${LIBNAME}.so
	rm -f mpi_proxy mpi_proxy.o

distclean: clean
	rm -f ${LIBNAME}.so *~ .*.swp dmtcp_restart_script*.sh ckpt_*.dmtcp

dist: distclean
	dir=`basename $$PWD`; cd ..; \
	  tar czvf $$dir.tar.gz --exclude-vcs ./$$dir
	dir=`basename $$PWD`; ls -l ../$$dir.tar.gz

.PHONY: default clean dist distclean
