# -----------------------------------------------------------------------
# Makefile for mini-redis
# -----------------------------------------------------------------------

CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11
TARGET  = mini-redis
SRCS    = server.c parser.c storage.c

# Default target: compile everything
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

# Run the server
run: all
	./$(TARGET)

# Remove compiled binary
clean:
	rm -f $(TARGET)

.PHONY: all run clean
