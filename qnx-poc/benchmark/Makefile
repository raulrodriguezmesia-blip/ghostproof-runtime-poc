CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lm

TARGET = latency_test
POLICY_ENGINE_DIR = ../policy_engine
SRCS = latency_test.c $(POLICY_ENGINE_DIR)/policy_eval.c
HDRS = $(POLICY_ENGINE_DIR)/policy_rules.h

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -I$(POLICY_ENGINE_DIR) -o $@ $(SRCS) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean