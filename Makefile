LIB_NAME = coreparse
TEST_BIN = coreparse_test

SRC_DIR = ./src
INC_DIR	= $(SRC_DIR)/include

CC = gcc
CFLAGS = -O3 -march=native -std=c11 -pedantic-errors -fPIC -I$(INC_DIR) -D_GNU_SOURCE
LDFLAGS = -lleveldb -lpthread -lm

# Source definitions
LIB_SRCS =	$(SRC_DIR)/coreparse_block.c		\
			$(SRC_DIR)/coreparse_context.c		\
			$(SRC_DIR)/coreparse_iterator.c		\
			$(SRC_DIR)/coreparse_debug_print.c	\
			$(SRC_DIR)/ctb_implementations.c	\
			$(SRC_DIR)/leveldb.c

LIB_OBJS = $(LIB_SRCS:.c=.o)
MAIN_SRC = main.c

# Output Library Names
STATIC_LIB = lib$(LIB_NAME).a
SHARED_LIB = lib$(LIB_NAME).so

# Targets
all: $(STATIC_LIB) $(SHARED_LIB) $(TEST_BIN)

# Link the test binary using the shared library
$(TEST_BIN): $(MAIN_SRC) $(SHARED_LIB)
	$(CC) $(CFLAGS) $(MAIN_SRC) -L. -l$(LIB_NAME) $(LDFLAGS) -Wl,-rpath,. -o $@

# Create Static Library (Archive)
$(STATIC_LIB): $(LIB_OBJS)
	ar rcs $@ $(LIB_OBJS)

# Create Shared Library
$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS)

# Compile Object Files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Build Artifacts
clean:
	rm -f $(LIB_OBJS) $(STATIC_LIB) $(SHARED_LIB) $(TEST_BIN)

.PHONY: all clean
