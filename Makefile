all: game_client game_server

game_client: game_client.cpp
	g++ -Wall -o game_client game_client.cpp -std=c++11 -lpthread
game_server: game_server.cpp
	g++ -Wall -o game_server game_server.cpp -std=c++11 -lpthread
clean:
	rm -f game_client game_server

