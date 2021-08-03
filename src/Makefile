CPP = g++
CPPFLAGS = -Wall -O3 -DFAST_BITBOARDS

BIN = r.exe

OBJ = bitboards.o   \
      eval.o        \
      eval_params.o \
      learn.o       \
      main.o        \
      moves.o       \
      notation.o    \
      position.o    \
      search.o      \
      utils.o       \
      win32.o

.PHONY: all clean

all: $(BIN)

clean:
	rm -rf $(BIN) $(OBJ)

$(BIN): $(OBJ)
	$(CPP) $(CPPFLAGS) $(OBJ) -o $(BIN)

.o: .cpp
	$(CPP) $(CPPFLAGS) -c $< -o $@

eval.o: eval_params.h
eval_params.o: eval_params.h
