CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
TARGET = log_tracker

OBJS = log_tracker.o parser.o analysis.o report.o socket_server.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf reports/

.PHONY: all clean