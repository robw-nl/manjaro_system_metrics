#!/bin/bash
PROJECT_NAME=${1:-$(basename "$PWD")}

cat <<EOF > makefile
CC = clang
# -MMD -MP tracks header dependencies automatically
CFLAGS = -Wall -Wextra -O2 -MMD -MP
TARGET = $PROJECT_NAME

# DYNAMIC FILE LISTS
# Finds all source (.c) and header (.h) files in the folder
SRCS = \$(wildcard *.c)
HDRS = \$(wildcard *.h)
OBJS = \$(SRCS:.c=.o)
DEPS = \$(SRCS:.c=.d)

# DEFAULT TARGET
all: \$(TARGET)

\$(TARGET): \$(OBJS)
	\$(CC) \$(CFLAGS) \$(OBJS) -o \$(TARGET)

# Include dependency files (ensures changing config.h triggers a rebuild)
-include \$(DEPS)

%.o: %.c
	\$(CC) \$(CFLAGS) -c $< -o \$@

# CLEANUP
clean:
	rm -f \$(TARGET) \$(OBJS) \$(DEPS)

# NUCLEAR OPTION: Use 'make rebuild' to force a fresh start
rebuild: clean all

.PHONY: all clean rebuild
EOF
echo "✅ Robust Makefile generated for $PROJECT_NAME."
