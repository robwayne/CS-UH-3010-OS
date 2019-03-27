minishell: main.o minishell.o
	g++ main.o minishell.o -o minishell

main.o: main.cpp
	g++ -c -std=c++11 main.cpp

minishell.o: minishell.cpp minishell.h
	g++ -c -std=c++11 minishell.cpp

cleanup:
	rm *.o minishell hist shvar

run:
	make minishell
	./minishell