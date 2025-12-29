# Makefile for arhint-signer (Windows with Visual Studio)
# Use with: nmake

CC = cl
CFLAGS = /EHsc /O2 /W3
LIBS = crypt32.lib ncrypt.lib bcrypt.lib advapi32.lib
TARGET = arhint-signer.exe
SOURCE = arhint-signer.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) /Fe:$(TARGET) $(SOURCE) $(LIBS)

clean:
	del /Q *.obj *.exe 2>nul

rebuild: clean all

.PHONY: all clean rebuild
