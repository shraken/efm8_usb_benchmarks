CC = sdcc
INCLUDE_DIRS = -Icode/include -Icode/source
SOURCE_DIR = code/source
BUILD_DIR = build
EXECUTABLE = usbhid_benchmark

SOURCES = $(shell find $(SOURCE_DIR) -type f -name '*.c')
OBJECTS = $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.rel, $(SOURCES))

CFLAGS = --model-large --std-sdcc99 -c
CFLAGS += $(INCLUDE_DIRS)

LDFLAGS = --model-large --std-sdcc99
LDFLAGS += $(INCLUDE_DIRS)

all: directories $(BUILD_DIR)/$(EXECUTABLE) generate
    
directories: 
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/drivers
	$(info $$SOURCES is [${SOURCES}])
	$(info $$OBJECTS is [${OBJECTS}])

$(BUILD_DIR)/$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@.ihx

$(OBJECTS): $(BUILD_DIR)/%.rel : $(SOURCE_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILD_DIR)

generate:
	packihx $(BUILD_DIR)/$(EXECUTABLE).ihx > $(BUILD_DIR)/$(EXECUTABLE).hex