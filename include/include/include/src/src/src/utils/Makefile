# Makefile per Router OS
CC = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -pthread -D_GNU_SOURCE
LDFLAGS = -lpthread -lm
DEBUG_FLAGS = -g -DDEBUG

# Directory
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Files sorgente
CORE_SOURCES = $(wildcard $(SRCDIR)/core/*.c)
NETWORK_SOURCES = $(wildcard $(SRCDIR)/network/*.c)
UTILS_SOURCES = $(wildcard $(SRCDIR)/utils/*.c)
MAIN_SOURCES = $(SRCDIR)/main.c $(SRCDIR)/logger.c

ALL_SOURCES = $(MAIN_SOURCES) $(CORE_SOURCES) $(NETWORK_SOURCES) $(UTILS_SOURCES)
OBJECTS = $(ALL_SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/router-os

# Configurazione
PREFIX ?= /usr/local
CONFDIR = /etc/router-os

.PHONY: all debug clean install uninstall test syntax-check

all: $(TARGET)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build completed: $(TARGET)"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled $<"

$(OBJDIR):
	@mkdir -p $(OBJDIR)/core $(OBJDIR)/network $(OBJDIR)/utils

$(BINDIR):
	@mkdir -p $(BINDIR)

# Installazione
install: $(TARGET)
	install -d $(PREFIX)/bin
	install -m 755 $(TARGET) $(PREFIX)/bin/
	install -d $(CONFDIR)
	install -d /var/log
	@echo "Installation completed"

uninstall:
	rm -f $(PREFIX)/bin/router-os
	rmdir $(CONFDIR) 2>/dev/null || true
	@echo "Uninstallation completed"

# Testing e validazione
test: syntax-check
	@echo "Syntax validation passed"

syntax-check:
	$(CC) $(CFLAGS) -fsyntax-only $(ALL_SOURCES)
	@echo "Syntax check completed successfully"

# Pulizia
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean completed"

# Cross-compilation info
cross-info:
	@echo "Cross-compilation setup:"
	@echo "Toolchain: $(CC)"
	@echo "Target architecture: ARM"
	@if command -v $(CC) >/dev/null 2>&1; then \
		echo "Toolchain found"; \
	else \
		echo "Warning: Toolchain not found. Please install arm-linux-gnueabihf-gcc"; \
	fi

# Documentazione
docs:
	@echo "Generating documentation..."
	@mkdir -p docs
	doxygen Doxyfile 2>/dev/null || echo "Doxygen not available or Doxyfile missing"

# Pacchetto distribuzione
package: clean all
	@echo "Creating distribution package..."
	tar -czf router-os-$(shell date +%Y%m%d).tar.gz src include Makefile README.md

# Help
help:
	@echo "Router OS Build System"
	@echo "====================="
	@echo "make            - Build release version"
	@echo "make debug      - Build debug version"
	@echo "make clean      - Clean build files"
	@echo "make install    - Install to system"
	@echo "make uninstall  - Remove from system"
	@echo "make test       - Run syntax validation"
	@echo "make cross-info - Check cross-compilation setup"
