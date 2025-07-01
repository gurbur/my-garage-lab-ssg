CC = gcc
CFLAGS = -g -Wall -Isrc/include

BUILD_DIR = builds
OBJ_DIR = $(BUILD_DIR)/obj
SRC_DIRS = src/tokenizer src/test

VPATH = $(SRC_DIRS)


TEST_SRCS = tokenizer.c test_tokenizer.c
TEST_OBJS = $(addprefix $(OBJ_DIR)/, $(TEST_SRCS:.c=.o))

TEST_TARGET = $(BUILD_DIR)/test_tokenizer


all: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	@echo "Linking executable: $@"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: %.c
	@echo "Compiling: $<"
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	@echo "Cleaning up build files..."
	rm -rf $(BUILD_DIR)

.PHONY: all clean

