CC = gcc
CFLAGS = -g -Wall
TARGET = minishell
SRCS = minishell.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)