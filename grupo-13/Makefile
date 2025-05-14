CC = gcc
CFLAGS = -Wall -Wextra -pedantic -g -Iinclude -D_GNU_SOURCE
LDFLAGS = 

# Alvos principais
all: folders bin/dserver bin/dclient

# Garante que os diretórios existam
folders:
	@mkdir -p src include obj bin tmp  

# Regra para o servidor (gera em bin/)
bin/dserver: obj/dserver.o obj/defs.o
	$(CC) $(LDFLAGS) $^ -o $@  

# Regra para o cliente (gera em bin/)
bin/dclient: obj/dclient.o
	$(CC) $(LDFLAGS) $^ -o $@  

# Compila .c -> .o
obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ 

# Limpeza
clean:
	rm -f obj/* tmp/* bin/*  