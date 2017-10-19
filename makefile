all:	 clean popser clean-obj

popser: main.cpp
	g++ -std=c++0x -Wall -Werror -c main.cpp
	g++ -std=c++0x -Wall -Werror main.o -o popser

clean-obj:
	rm -rf *.o

clean:
	rm -rf popser