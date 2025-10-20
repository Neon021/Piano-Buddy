CC = gcc

CFLAGS = -Wall -Iinclude -g

REC_APP = bin/record_app
ANALYZE_APP = bin/analyze_app

REC_SRC = src/recorder.c
REC_OBJ = $(REC_SRC:.c=.o)

ANALYZE_SRC = src/analyzer.c
ANALYZE_OBJ = $(ANALYZE_SRC:.c=.o)

# Base libs for both
LDFLAGS_BASE = -lportaudio -lsndfile
# Libs needed only for the analyzer
# -lfftw3 is for the FFTW library
# -lm is the math library (for sqrt())
LDFLAGS_ANALYZE = $(LDFLAGS_BASE) -lfftw3 -lm

# Build Rules
# 'all' is the default rule. It now builds both apps.
all: $(REC_APP) $(ANALYZE_APP)

# Rule to build the recorder
$(REC_APP): $(REC_OBJ)
	@echo "Linking Recorder..."
	$(CC) $(REC_OBJ) -o $(REC_APP) $(LDFLAGS_BASE)
	@echo "Recorder build finished: $(REC_APP)"

# Rule to build the analyzer
$(ANALYZE_APP): $(ANALYZE_OBJ)
	@echo "Linking Analyzer..."
	$(CC) $(ANALYZE_OBJ) -o $(ANALYZE_APP) $(LDFLAGS_ANALYZE)
	@echo "Analyzer build finished: $(ANALYZE_APP)"

# Generic rule to compile any .c file in src/ into its .o file
src/%.o: src/%.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up
clean:
	@echo "Cleaning up..."
	rm -f src/*.o $(REC_APP) $(ANALYZE_APP)