TARGET_EXEC ?= a.out

BUILD_DIR ?= build/linux
SRC_DIRS ?= src

SRCS := $(shell find $(SRC_DIRS) -name "*.cpp" -or -name "*.c" -or -name "*.s")
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

ADDITIONAL_OBJS := 
#thirdparty/glad/glad.o


INC_DIRS := $(shell find $(SRC_DIRS) -type d) include/
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
LDFLAGS := -lGL -lglfw3 -lX11 -lpthread -ldl

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -std=c++17

$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) ${ADDITIONAL_OBJS} -o $@ $(LDFLAGS)

# assembly
$(BUILD_DIR)/%.s.o: %.s
	$(MKDIR_P) $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cpp.o: %.cpp
	$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

run:
	build/linux/a.out


-include $(DEPS)

MKDIR_P ?= mkdir -p