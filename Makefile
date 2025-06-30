# compile and install a simple ls command
CC = gcc
CFLAGS = -Wall -O2

SRC = ls.c
TARGET = ls

# the installation prefix
PREFIX := $(HOME)/.local/bin

# default run the makefile
all: $(TARGET) install clean

# compile
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

# install
install: $(TARGET)
	@mkdir -p $(PREFIX)
	cp $(TARGET) $(PREFIX)/
	chmod 755 $(PREFIX)/$(TARGET)

# clean
clean:
	rm -f $(TARGET)
