#!/bin/bash
PROJECT_NAME=${1:-$(basename "$PWD")}

# --- SMART LIBRARY DISCOVERY ---
LIBS=""
grep -q "alsa/asoundlib.h" *.c 2>/dev/null && LIBS="$LIBS -lasound"
grep -q "pulse/pulseaudio.h" *.c 2>/dev/null && LIBS="$LIBS -lpulse"
grep -q "math.h" *.c 2>/dev/null && LIBS="$LIBS -lm"

cat <<EOF > makefile
CC = clang
CFLAGS = -Wall -Wextra -O2 -MMD -MP
TARGET = $PROJECT_NAME
LDLIBS = $LIBS

# DYNAMIC FILE LISTS
SRCS = \$(wildcard *.c)
OBJS = \$(SRCS:.c=.o)
DEPS = \$(SRCS:.c=.d)

# DEFAULT TARGET
all: \$(TARGET)

\$(TARGET): \$(OBJS)
	\$(CC) \$(CFLAGS) \$(OBJS) -o \$(TARGET) \$(LDLIBS)
	@rm -f \$(OBJS) \$(DEPS)
	@echo "--- Build successful: Binary '\$(TARGET)' preserved, clutter purged ---"

# Include dependency files (valid until the purge at the end of build)
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

echo "✅ Smart Makefile generated for $PROJECT_NAME with LIBS:$LIBS"
