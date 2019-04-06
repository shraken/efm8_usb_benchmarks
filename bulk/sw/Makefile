#OS = LINUX
#OS = MACOSX
OS = WINDOWS
BUILD_DIR = build

ifeq ($(OS), LINUX)
CC = gcc
CFLAGS = -Wall -O2 -DLINUX
TARGET = receive_test

else ifeq ($(OS), MACOSX)
CC = gcc
CFLAGS = -Wall -O2 -DMACOSX
TARGET = receive_test

else ifeq ($(OS), WINDOWS)
#CC = i586-mingw32msvc-gcc
CC = mingw32-gcc-6.3.0
CFLAGS = -Wall -O2 -DWINDOWS
TARGET = receive_test.exe
endif

all: $(TARGET)

$(TARGET): receive_test.c Makefile
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET) receive_test.c

clean:
	rm -rf $(BUILD_DIR)

