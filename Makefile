all:
	g++ -I src/include -L src/lib -o chip8 chip8.cpp -lmingw32 -lSDL2main -lSDL2