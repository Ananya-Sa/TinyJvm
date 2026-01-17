# Toolchain
CC := musl-gcc

# Flags
CFLAGS  := -O2 -static -Wall -Wextra -std=c11
CPPFLAGS :=
LDFLAGS :=
LDLIBS  :=

BIN_JAVA  := java
BIN_JAVAC := javac

SRC_COMMON := \
  src/common/arena.c \
  src/common/diag.c \
  src/common/lexer.c \
  src/common/parser.c \
  src/common/str.c

SRC_JAVAC := \
  src/javac/main.c \
  src/javac/emit_class.c

SRC_JAVA := \
  src/java/main.c \
  src/java/classfile.c \
  src/java/vm.c

OBJS_COMMON := $(SRC_COMMON:.c=.o)
OBJS_JAVAC  := $(SRC_JAVAC:.c=.o)
OBJS_JAVA   := $(SRC_JAVA:.c=.o)

.PHONY: all clean

all: $(BIN_JAVA) $(BIN_JAVAC)

# Compile
src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Link (javac uses common + javac)
$(BIN_JAVAC): $(OBJS_COMMON) $(OBJS_JAVAC)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# Link (java likely also needs common)
$(BIN_JAVA): $(OBJS_COMMON) $(OBJS_JAVA)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(BIN_JAVA) $(BIN_JAVAC) $(OBJS_COMMON) $(OBJS_JAVAC) $(OBJS_JAVA)
