CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

SRCS = main.c node_manager.c replication.c fault_tolerance.c
OBJS = $(SRCS:.c=.o)
TARGET = dfs

ifeq ($(OS),Windows_NT)
	TARGET := $(TARGET).exe
	RM = del /Q /F
else
	RM = rm -f
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c dfs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.o $(TARGET)

run: $(TARGET)
	./$(TARGET)
