CC = gcc
CFLAGS = -g -Wall -Isrc/include -Isrc/libs/cjson -Isrc/libs/sha256

BUILD_DIR = builds
OBJ_DIR = $(BUILD_DIR)/obj

SRC_DIRS = src \
					 src/libs/cjson src/libs/sha256 \
					 src/utils tests/src src/builder \
					 src/tokenizer src/parser src/html_generator src/template_engine \

VPATH = $(SRC_DIRS)

LIB_SRCS = \
          build_process.c \
					site_context.c \
          token_handlers.c tokenizer.c \
          parser_utils.c inline_parser.c block_parser.c parser.c \
          html_generator.c node_renderer.c \
          template_engine.c context_manager.c template_utils.c \
          dynamic_buffer.c \
          hash_table.c \
          file_utils.c \
					hash_utils.c \
          config_loader.c \
          ignore_handler.c \
					cache_manager.c \
          cJSON.c sha256.c

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
