# Makefile — K-Moyennes en C modulaire
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lm

SRC     = src/pgm_io.c src/kmeans.c src/main.c
TARGET  = kmeans

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

run: $(TARGET)
	@mkdir -p output
	./$(TARGET)

clean:
	rm -f $(TARGET)
