CC = gcc
CFLAGS = -g -Wall

SERVER_SRC = server.c handle.c hash.c helpers.c message.c
SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_TARGET = server

SUBSCRIBER_SRC = client.c helpers.c
SUBSCRIBER_OBJ = $(SUBSCRIBER_SRC:.c=.o)
SUBSCRIBER_TARGET = subscriber

.PHONY: all clean

all: $(SERVER_TARGET) $(SUBSCRIBER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJ)
	$(CC) $(CFLAGS) $(SERVER_OBJ) -o $(SERVER_TARGET)

$(SUBSCRIBER_TARGET): $(SUBSCRIBER_OBJ)
	$(CC) $(CFLAGS) $(SUBSCRIBER_OBJ) -o $(SUBSCRIBER_TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJ) $(SERVER_TARGET) $(SUBSCRIBER_OBJ) $(SUBSCRIBER_TARGET)