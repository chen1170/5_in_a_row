# Compiler
CC = mpicc

# Compiler flags
CFLAGS = -Wall -Wextra -O2

# Executable name
TARGET = game

# Source files
SRCS = main.c board.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HDRS = board.h

# Default target
all: $(TARGET)

# Link object files to create the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile source files into object files
%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program with MPI
run: $(TARGET)
	mpirun -np 4 ./$(TARGET)

.PHONY: all clean run
