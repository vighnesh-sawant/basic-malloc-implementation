malloc.so: malloc.c
	clang -O0 -g -W -Wall -Wextra -shared -fPIC malloc.c -o malloc.so

TESTS=$(wildcard tests/*.c)
BINS=$(TESTS:tests/%.c=%)

test: malloc.so $(BINS) 
	@for n in $(BINS); do \
			bash tests/run.sh $$n; \
    done
%: tests/%.c
	clang -o tests/$@ $<
