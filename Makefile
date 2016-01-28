all: sample2D
sample2D: game.cpp glad.c
	 g++ -o game game.cpp -L/usr/local/lib/ -lglfw glad.c -lGL -lglfw -ldl
clean:
	rm sample2D sample3D
