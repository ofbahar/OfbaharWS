all: sozluk kuyruk server final

sozluk: sozluk.c
	gcc -Wall -g -std=gnu99 -o sozluk.o -c sozluk.c -lpthread

kuyruk: kuyruk.c
	gcc -Wall -g -std=gnu99 -o kuyruk.o -c kuyruk.c

server: server.c
	gcc -Wall -g -std=gnu99 -o server.o -c server.c -lpthread

final: server.c
	gcc -Wall -g -std=gnu99 -o sunucu server.o sozluk.o kuyruk.o -lpthread

clean:
	- rm -rf *.o $(objects) sunucu kuyruk.c~ sozluk.c~ kuruk.h~ sozluk.c~
