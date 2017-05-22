CC = gcc
CFLAGS = -c -Wall

demon: daemon.o container.o
	$(CC) -o demon daemon.o container.o && sudo ./demon

app: app.o container.o
	$(CC) -o cli app.o container.o

app.o:
	$(CC) $(CFLAGS) app.c

daemon.o:
	$(CC) $(CFLAGS) daemon.c 

container.o:
	$(CC) $(CFLAGS) container.c

clean:
	rm -rf *.o *.out
