CFLAGS=-Wall -Wextra -Werror -O2
TARGETS=lab1test lab1pueN3247 libpueN3247.so

.PHONY: all clean

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS)

lab1test: lab1test.c plugin_api.h
	gcc $(CFLAGS) -o lab1test lab1test.c -ldl

lab1pueN3247: lab1pueN3247.c plugin_api.h
	gcc $(CFLAGS) -o lab1pueN3247 lab1pueN3247.c -ldl

libpueN3247.so: libpueN3247.c plugin_api.h
	gcc $(CFLAGS) -shared -fPIC -o libpueN3247.so libpueN3247.c -ldl -lm
