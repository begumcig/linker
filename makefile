CC=g++
CXXFLAGS=-I. -std=c++11
DEPS = Tokenizer
OBJ = Tokenizer.o lab1.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CXXFLAGS)

linker: $(OBJ)
	$(CC) -o  $@ $^ $(CXXFLAGS)
	make clean
clean:
	rm -f $(OBJ) 
