CC = ../../../MinGW/bin/gcc.exe
CFLAGS = -Wall -Wextra -O2 -Iinclude -I../../../MinGW/include
LDFLAGS = -L../../../MinGW/lib -lOpenCL

TARGET = mandelbrot.exe
SOURCES = src/main.c src/cl_utils.c src/kernel_loader.c src/image_writer.c

all:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	del /Q $(TARGET) 2>nul || true