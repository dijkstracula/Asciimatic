CC=gcc
CFLAGS=-g -Wall -Wextra -std=gnu99
LDFLAGS=-lconfig `pkg-config --libs opencv` `pkg-config --libs cairo`
SRCDIR=src

SOURCES=$(wildcard $(SRCDIR)/*.c)
OBJECTS=$(SOURCES:.c=.o)

TARGET=asciimatic

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(OBJECTS)
	rm $(TARGET)
