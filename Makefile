CC      ?= gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -pedantic
PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin

TARGET  = jpp
SRC     = jpp.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all install uninstall clean

