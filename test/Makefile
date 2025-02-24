# filepath: test/Makefile
CFLAGS  = -Wall -std=c99 -I../include -I.
LDFLAGS = -lcunit
# If CUnit is in a non-standard location, add the path here
# For example, if CUnit is in /opt/cunit:
# LDFLAGS = -L/opt/cunit/lib -lcunit

test_strutil: test_strutil.c ../src/strutil.c
	gcc $(CFLAGS) test_strutil.c ../src/strutil.c -o test_strutil $(LDFLAGS) -pthread

test: test_strutil
	./test_strutil

clean:
	rm -f test_strutil