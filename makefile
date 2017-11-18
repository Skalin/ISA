all:	 clean popser clean-obj

popser: main.cpp md5.h main.h
	g++ -std=c++0x -Wall -Werror -pthread -c md5.cpp
	g++ -std=c++0x -Wall -Werror -pthread -c main.cpp
	g++ -std=c++0x -Wall -Werror -pthread main.o md5.o -o popser

clean-obj:
	rm -rf *.o

clean:
	rm -rf popser
