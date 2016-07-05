CFLAGS=-g -O2 -Wall -Wno-deprecated
CXXFLAGS=-g -O2 -Wall -Wno-deprecated

all: track-branch-func.a

track-branch-func.a: track-branch-func.o
	ar rcs $@ $^

%.o: %.cpp
	gcc -o $@ $< $(CXXFLAGS) -c -MMD

clean:
	rm -f *.a *.o *.d

