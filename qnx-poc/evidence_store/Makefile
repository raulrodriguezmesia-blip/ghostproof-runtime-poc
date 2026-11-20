CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lsqlite3

TARGET = evidence_store
SRCS = main.c evidence_store.c
HDRS = evidence_store.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) evidence.db evidence.db-wal evidence.db-shm

.PHONY: all run clean