# Not using LLVM's Makefile.common 

all: install/bin/stp

install/bin/stp: stp-svn
	cd stp-svn && ./clean-install.sh --with-g++='g++ -fPIC' --with-gcc='gcc -fPIC' --with-prefix=`pwd`/../install

stp-svn:
	svn co -r 1656 https://stp-fast-prover.svn.sourceforge.net/svnroot/stp-fast-prover/trunk/stp stp-svn
	
test:
	cd stp-svn && echo `pwd`

update: stp-svn
	cd stp-svn && svn update && make install

clean:
	make -C stp-svn clean

dist-clean: clean
	rm -rf install
