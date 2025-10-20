CC = gcc

CFLAGS = -Wall -Iinclude -g

LDFLAGS = -lportaudio -lsndfile

EXECUTABLE=bin/piano_buddy

SOURCES=$(wildcard src/*.c)

OBJECTS=$(SOURCES:.c=.o)

# 3. Build Rules
# The first rule is the default one that runs when you just type 'make'
all: $(EXECUTABLE)

# Rule to create the final executable
$(EXECUTABLE): $(OBJECTS)
	@echo "Linking..."
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)
	@echo "Build finished. Run with: ./$(EXECUTABLE)"

# Rule to compile .c source files into .o object files
# $< is the source file name (e.g., src/main.c)
# $@ is the target file name (e.g., src/main.o)
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up compiled files
clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(EXECUTABLE)