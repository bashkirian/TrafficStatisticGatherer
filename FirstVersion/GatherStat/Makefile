CC=gcc
CC_FLAGS=-g -Wall
CC_LIBS= -pthread
SRC_DIR=src
HDR_DIR=include/
OBJ_DIR=obj

# source and object files
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

BIN_FILE=GatherStat

all: $(OBJ_DIR) $(BIN_FILE)

$(BIN_FILE): $(OBJ_FILES)
	$(CC) $(CC_FLAGS) $^ -I$(HDR_DIR) -o $@ $(CC_LIBS) -lrt

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CC_FLAGS) -c $^ -I$(HDR_DIR) -o $@ $(LFLAGS)

$(OBJ_DIR):
	mkdir $@

clean:
	rm -rf $(BIN_FILE) $(OBJ_DIR)
