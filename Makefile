CFLAGS = -Wall
all: server subscriber

# Compileaza server.c
server: 
	gcc server.c -o server $(CFLAGS)
# Compileaza client.c
subscriber:
	gcc subscriber.c -o subscriber $(CFLAGS)


clean:
	rm -f server subscriber