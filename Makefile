all: 
	gcc -o ./bin/memory_server server.c ringbuffer.c -lrt -pthread -g -lm
	gcc -o ./bin/client client.c ringbuffer.c -lrt -pthread -g -lm

debug: 
	gcc -o ./bin/memory_server server.c ringbuffer.c -lrt -pthread -g -lm -DDEBUG
	gcc -o ./bin/client client.c ringbuffer.c -lrt -pthread -g -lm -DDEBUG
	
clean:
	rm -f ./read.*
	rm -f ./sectors.*
	rm -f ./bin/memory_server
	rm -f ./bin/client
	rm -f ./disk1.img
