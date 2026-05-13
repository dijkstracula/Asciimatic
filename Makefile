CC=gcc
CXX=g++

PKG_CFLAGS=`pkg-config --cflags opencv4 cairo libconfig`
PKG_LIBS=`pkg-config --libs opencv4 cairo libconfig`

CFLAGS=-g -Wall -Wextra -std=gnu99 $(PKG_CFLAGS)
CXXFLAGS=-g -Wall -Wextra -std=gnu++17 $(PKG_CFLAGS)
LDFLAGS=$(PKG_LIBS)

SRCDIR=src

# Files that are now C++ internally, kept with .c extension for git-history
# continuity.  Compiled explicitly with $(CXX) -x c++.
CXX_SOURCES=$(SRCDIR)/asciimatic.c $(SRCDIR)/gui.c
C_SOURCES=$(filter-out $(CXX_SOURCES), $(wildcard $(SRCDIR)/*.c))

CXX_OBJECTS=$(CXX_SOURCES:.c=.o)
C_OBJECTS=$(C_SOURCES:.c=.o)
OBJECTS=$(C_OBJECTS) $(CXX_OBJECTS)

TARGET=asciimatic

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(C_OBJECTS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(CXX_OBJECTS): %.o : %.c
	$(CXX) $(CXXFLAGS) -x c++ -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
