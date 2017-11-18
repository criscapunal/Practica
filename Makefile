all:
	gcc -c src/p2-dogServer.c -o obj/server
	gcc -o bin/servidor obj/server -pthread
	gcc -c src/p2-dogClient.c -o obj/client
	gcc -o bin/client/cliente obj/client -pthread



	
