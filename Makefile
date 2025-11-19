malloc.so: malloc.c
	clang -O0 -g -W -Wall -Wextra -shared -fPIC malloc.c -o malloc.so

install:
	export DYLD_INSERT_LIBRARIES=/home/vighnesh/workspace/basic-malloc-implementation/malloc.so
