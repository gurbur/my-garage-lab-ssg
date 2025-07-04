CC = gcc
CFLAGS = -g -Wall -Isrc/include

BUILD_DIR = builds
OBJ_DIR = $(BUILD_DIR)/obj
SRC_DIRS = src/tokenizer src/parser tests/src src

VPATH = $(SRC_DIRS)


LIB_SRCS = tokenizer.c parser.c
LIB_OBJS = $(addprefix $(OBJ_DIR)/, $(LIB_SRCS:.c=.o))

MAIN_SRC = main.c
MAIN_OBJ = $(addprefix $(OBJ_DIR)/, $(MAIN_SRC:.c=.o))

TEST_C_FILES = $(wildcard tests/src/*.c)
TEST_NAMES = $(notdir $(TEST_C_FILES:.c=))
TEST_TARGETS = $(addprefix $(BUILD_DIR)/, $(TEST_NAMES))

SSG_TARGET = $(BUILD_DIR)/ssg


all: $(SSG_TARGET)

ssg: $(SSG_TARGET)

test: $(TEST_TARGETS)
	@echo "==> Running all tests via script..."
	@./tests/run_tests.sh

$(SSG_TARGET): $(MAIN_OBJ) $(LIB_OBJS)
	@echo "==> Linking Main Executable: $@"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%: $(OBJ_DIR)/%.o $(LIB_OBJS)
	@echo "==> Linking Test Executable: $@"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: %.c
	@echo "Compiling: $<"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up build files..."
	rm -rf $(BUILD_DIR)

.PHONY: all ssg test clean
