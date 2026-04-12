CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lOpenCL

TARGET = mandelbrot.exe
SOURCES = src/main.c src/cl_utils.c src/kernel_loader.c

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
