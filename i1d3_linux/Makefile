# i1d3_linux Makefile
# Version: 1.0.0
# Build system for i1d3 Linux HID communication library

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -std=c99 -pedantic
LDFLAGS = -lm
TARGET = i1d3_test
SOURCES = main.c i1d3.c
HEADERS = i1d3.h i1d3_version.h
OBJECTS = $(SOURCES:.c=.o)

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 0
VERSION_STRING = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)

.PHONY: all clean install uninstall test help version

# Default target
all: $(TARGET)

# Build the main executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install the executable (optional)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall the executable
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Build and run basic test
test: $(TARGET)
	@echo "Testing compilation only (requires hardware for full test)"
	./$(TARGET) --help 2>/dev/null || echo "No --help flag implemented yet"

# Show version information
version:
	@echo "i1d3_linux version $(VERSION_STRING)"
	@echo "Major: $(VERSION_MAJOR), Minor: $(VERSION_MINOR), Patch: $(VERSION_PATCH)"

# Show help
help:
	@echo "i1d3_linux Build System v$(VERSION_STRING)"
	@echo ""
	@echo "Available targets:"
	@echo "  all      - Build the executable (default)"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install executable to /usr/local/bin"
	@echo "  test     - Build and run basic test"
	@echo "  version  - Show version information"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Usage: make [target]"