malloc.so: malloc.c
	clang -O0 -g -W -Wall -Wextra -shared -fPIC malloc.c -o malloc.so

TESTS=$(wildcard tests/*.c)
BINS=$(TESTS:tests/%.c=%)

test: malloc.so $(BINS) 
	@for n in $(BINS); do \
			bash tests/run.sh tests/$$n; \
    done
	bash tests/run.sh /bin/ls;
	bash tests/run.sh firefox;
%: tests/%.c
	clang -o tests/$@ $<

