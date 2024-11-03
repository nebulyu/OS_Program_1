CFLAGS=-Wall -Werror -g 

TARGETS=server client

all: $(TARGETS)

server: server.o
	$(CC) $(CFLAGS) -o $@ $<

client: client.o
	$(CC) $(CFLAGS) -o $@ $<


%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(TARGETS) *.o

