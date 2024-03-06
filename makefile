CC = gcc
AR = ar
FLAG = -Wall -g -fPIC

#compile and link all the files
all: TCP_Program RUDP_Program

TCP_Program: TCP_Receiver TCP_Sender

RUDP_Program: RUDP_Receiver RUDP_Sender


#compile program
TCP_Receiver: TCP_Receiver.o
	$(CC) $(FLAG) -o TCP_Receiver TCP_Receiver.o

TCP_Sender: TCP_Sender.o
	$(CC) $(FLAG) -o TCP_Sender TCP_Sender.o

RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(FLAG) -o RUDP_Receiver RUDP_Receiver.o RUDP_API.o

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(FLAG) -o RUDP_Sender RUDP_Sender.o RUDP_API.o


#compile object files
TCP_Receiver.o: TCP_Receiver.c
	$(CC) $(FLAG) -c TCP_Receiver.c -o TCP_Receiver.o

TCP_Sender.o: TCP_Sender.c
	$(CC) $(FLAG) -c TCP_Sender.c -o TCP_Sender.o

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h
	$(CC) $(FLAG) -c RUDP_Receiver.c -o RUDP_Receiver.o

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(FLAG) -c RUDP_Sender.c -o RUDP_Sender.o

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(FLAG) -c RUDP_API.c -o RUDP_API.o


#to ensure that there isnt a "clean"/"all" file in the directory
.PHONY: clean all

#clean all the compiled files and leave the .txt/ .c / .h files
clean:
	rm -f *.o *.a *.so TCP_Receiver TCP_Sender RUDP_Receiver RUDP_Sender