CC=gcc
CFLAGS=-g -Wall -Wextra
LDFLAGS=-lconfig `pkg-config --libs opencv` `pkg-config --libs cairo`
SRCDIR=src

SOURCES=$(wildcard $(SRCDIR)/*.c)
OBJECTS=$(SOURCES:.c=.o)

TARGET=asciimatic

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(TARGET)
	rm $(OBJECTS)
