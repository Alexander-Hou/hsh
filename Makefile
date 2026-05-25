CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -std=c11 -g -O2
LDFLAGS := -lm

SRCDIR := src
INCDIR := include
OBJDIR := build
BINDIR := bin
TARGET := hsh

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all clean distclean dirs

all: dirs $(BINDIR)/$(TARGET)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o

distclean: clean
	rm -rf $(BINDIR)

# Convenience
.DEFAULT_GOAL := all
