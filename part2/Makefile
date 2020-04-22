http-server: http-server.o
	gcc -g -Wall http-server.o -o http-server

http-server.o: http-server.c
	gcc -c -g -Wall http-server.c

.PHONY: vtest
vtest: http-server
	valgrind --leak-check=yes ./http-server 11122 ~/html localhost 11123
	
.PHONY: clean
clean: 
	rm -f *.o http-server
