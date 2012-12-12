Project: src/player_space.c src/input.c src/sprite.c src/init.c src/definitions.h
	gcc -o Player src/player_space.c src/input.c src/sprite.c -lSDL -lSDL_ttf
	gcc -o Server src/init.c src/input.c src/sprite.c -lSDL -lSDL_ttf

clean:
	rm src/*.o src/*~ src/*#

cclean:
	make clean; rm Player Server