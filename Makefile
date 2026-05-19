CC = gcc
CFLAGS = -Wall -pthread

SRCS = log_tracker.c parser.c analysis.c report.c socket_server.c
TARGET = log_tracker

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) summary.txt incident.txt