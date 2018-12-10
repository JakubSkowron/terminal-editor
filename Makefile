.PHONY: all clean

all: editor

clean:
	rm editor

editor: main.cpp
	g++ --std=c++14 -Wall -Werror -o editor main.cpp

