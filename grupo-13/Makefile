CC = gcc
CFLAGS = -Wall -g -Iinclude -D_GNU_SOURCE
LDFLAGS = 

all: folders dserver dclient

dserver: dserver

dclient: dclient

folders:
	@mkdir -p src include obj bin tmp 

dserver: obj/dserver.o 
	$(CC) $(LDFLAGS) $^ -o $@

dclient: obj/dclient.o 
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*