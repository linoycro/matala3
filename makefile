CC=gcc
FLAGS=-Wall -g
OBJFILES=RUDP_Sender.o RUDP_Receiver.o RUDP_API.o
RUDP = RUDP_Receiver RUDP_Sender 


.PHONY: all clean

all: rudp

rudp: $(RUDP)

RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(FLAGS) $^ -o $@	

RUDP_Receiver.o: RUDP_Receiver.c RUDP_API.h 
	$(CC) $(FLAGS) -c $<	

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(FLAGS) $^ -o $@	

RUDP_Sender.o: RUDP_Sender.c RUDP_API.h
	$(CC) $(FLAGS) -c $<	

RUDP_API.o: RUDP_API.c RUDP_API.h
	$(CC) $(CFLAGS) -c RUDP_API.c

clean:
	rm -f $(OBJFILES) $(TARGET)
