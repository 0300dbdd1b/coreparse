LIB_NAME = coreparse
TEST_BIN = coreparse_test

SRC_DIR = ./src
INC_DIR	= $(SRC_DIR)/include

CC = gcc
CFLAGS = -std=c11 -pedantic-errors -O3 -march=native -mtune=native -flto -fPIC -I$(INC_DIR) -D_GNU_SOURCE
LDFLAGS = -lleveldb -lpthread -lm 

DEBUG_CFLAGS	= $(CFLAGS) -g -fsanitize=address
DEBUG_LDFLAGS	= $(LDFLAGS) -fsanitize=address

LIB_SRCS =	$(SRC_DIR)/coreparse_block.c		\
			$(SRC_DIR)/coreparse_context.c		\
			$(SRC_DIR)/coreparse_iterator.c		\
			$(SRC_DIR)/coreparse_debug_print.c	\
			$(SRC_DIR)/ctb_implementations.c	\
			$(SRC_DIR)/leveldb.c

LIB_OBJS = $(LIB_SRCS:.c=.o)

TEST_SRC = test.c

STATIC_LIB = lib$(LIB_NAME).a
SHARED_LIB = lib$(LIB_NAME).so

PYEXEC	=	python3

all: $(STATIC_LIB) $(SHARED_LIB) $(TEST_BIN)

python: $(SHARED_LIB)
	$(PYEXEC) -m pip install -e .
	$(PYEXEC) setup.py build_ext --inplace

$(TEST_BIN): $(MAIN_SRC) $(SHARED_LIB)
	$(CC) $(DEBUG_CFLAGS) $(TEST_SRC) -L. -l$(LIB_NAME) $(LDFLAGS) -Wl,-rpath,. -o $@

$(STATIC_LIB): $(LIB_OBJS)
	ar rcs $@ $(LIB_OBJS)

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) -shared -o $@ $(LIB_OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJS) $(TEST_BIN)

fclean: clean
	rm -f $(STATIC_LIB) $(SHARED_LIB)
	rm -rf $(LIB_NAME).egg-info
	rm -f *.so
	rm -rf build

.PHONY: all clean
