# NexOS Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
LDFLAGS = -pthread

# Directories
SRC_DIR = .
OBJ_DIR = build
BIN_DIR = bin

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c) \
       $(wildcard $(SRC_DIR)/kernel/*.c) \
       $(wildcard $(SRC_DIR)/memory/*.c) \
       $(wildcard $(SRC_DIR)/scheduler/*.c) \
       $(wildcard $(SRC_DIR)/io/*.c) \
       $(wildcard $(SRC_DIR)/ai_engine/*.c) \
       $(wildcard $(SRC_DIR)/security/*.c) \
       $(wildcard $(SRC_DIR)/webserver/*.c) \
       $(wildcard $(SRC_DIR)/drivers/*.c) \
       $(wildcard $(SRC_DIR)/lib/*.c) \
       $(wildcard $(SRC_DIR)/utils/*.c)

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Output binaries
BIN = $(BIN_DIR)/nexos.bin
WEBSERVER = $(BIN_DIR)/nexos_webserver

# Phony targets
.PHONY: all clean webserver run

# Default target
all: $(BIN) $(WEBSERVER)

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)
	mkdir -p $(OBJ_DIR)/kernel
	mkdir -p $(OBJ_DIR)/memory
	mkdir -p $(OBJ_DIR)/scheduler
	mkdir -p $(OBJ_DIR)/io
	mkdir -p $(OBJ_DIR)/ai_engine
	mkdir -p $(OBJ_DIR)/security
	mkdir -p $(OBJ_DIR)/webserver
	mkdir -p $(OBJ_DIR)/drivers
	mkdir -p $(OBJ_DIR)/lib
	mkdir -p $(OBJ_DIR)/utils

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object files for main NexOS binary
$(BIN): $(filter-out $(OBJ_DIR)/nexos_webserver.o,$(OBJS)) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

# Build NexOS Web Server
$(WEBSERVER): $(OBJ_DIR)/nexos_webserver.o $(filter-out $(OBJ_DIR)/main.o,$(OBJS)) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Webserver target
webserver: $(WEBSERVER)

# Run NexOS Web Server
run: $(WEBSERVER)
	$(WEBSERVER) -p 8080 -r ./webroot

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Include dependencies
-include $(OBJS:.o=.d)