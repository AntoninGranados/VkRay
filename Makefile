CC = clang
LIB = -L$(VULKAN_SDK)/lib
LINK = -lc++ -lglfw3 -lvulkan -Wl,-rpath,$(VULKAN_SDK)/lib
INCLUDE = -I/usr/local/include -Iinclude/ -Iinclude/imgui -I$(VULKAN_SDK)/include
FLAGS = -Wall -O3 -std=c++23 -arch arm64 # -DNDEBUG
FRAMEWORKS = -framework Accelerate -framework Cocoa -framework IOKit -framework Metal -framework QuartzCore -F/usr/local/Cellar/molten-vk/1.2.11/Frameworks

SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BUILD_DIR)/app

SRC = $(shell find $(SRC_DIR) -name '*.cpp')
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%.o)
DEPS := $(OBJ:.o=.d)

$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR))

all: $(TARGET)

$(TARGET): $(OBJ)
	@echo "[LINKING]" ...
	@$(CC) $(LIB) $(INCLUDE) $(FLAGS) $(FRAMEWORKS) $(LINK) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@echo "[BUILDING]" $<
	@mkdir -p $(dir $@)
	@$(CC) $(INCLUDE) $(FLAGS) -MMD -MP -MF $(@:.o=.d) -o $@ -c $<

-include $(DEPS)

clean:
	@echo "[CLEANING]"
	@rm -rf $(BUILD_DIR)/* $(BIN_DIR)/*
