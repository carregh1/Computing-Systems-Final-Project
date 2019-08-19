/* Diego Carregha
   Computing Systems
   Final Project
   game_client.cpp
*/
#include <iostream>
#include <string>
#include <cstring>
#include <ctype.h>
#include <sys/types.h>  
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>  
#include <unistd.h>     
#include "P4.h"

using namespace std;


int main(int argc, char *argv[])
{
	if (argc != 3){
		cout << "Not enough arguments, please enter port and IP." << endl;
		return 0;
	}
	
	unsigned long serverIP;
	//convert IP from char to binary
    int status = inet_pton(AF_INET, argv[1], (void *)&serverIP);

	//convert port from char to short int
    unsigned short serverPort = stoi(argv[2]);
	//create socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1){
		cout << "Error creating socket." << endl;
		return 0;
	}

	//create socket addr struct to give socket an addr for communication
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = serverIP; //give IP
    serverAddress.sin_port = htons(serverPort); //htons converts host short to network byte

	//connect to the server at given addr
    status = connect(serverSocket, (sockaddr *)&serverAddress, sizeof(serverAddress));
    if (status != 0){
        cout << "Error connecting to server." << endl;
        return 0;
    } else {
		//welcome message
		cout << endl << "Connected to game server, welcome to Hangman!" << endl;
	}

    //get username
    string username;
    cout << "Please enter a username: ";
    getline(cin, username);
    cout << endl;

    //send username to server
    sendLong(serverSocket, long(username.length()));
    sendString(serverSocket, username);
	
	//start game
	bool finished = false;
	long wordLength;
	string wordPlay;
	long result;
	string guess;
	bool valid;
	long turns;
	while (!finished){
		//get turns
		turns = recvLong(serverSocket);
		if (turns == -1){
			cout << "Error receiving turns." << endl;
			return 0;
		}
		cout << endl << "Turn: " << turns << endl;
		
		//get word
		wordLength = recvLong(serverSocket);
		if (wordLength == -1){
			cout << "Error receiving word length." << endl;
			return 0;
		}
		
		wordPlay = recvString(serverSocket, wordLength);
		if (wordPlay == "-1"){
			cout << "Error receiving word to play." << endl;
			return 0;
		}
		
		cout << "Word: " << wordPlay << " "<< endl;
		
		valid = false;
		//ask for guess
		while (!valid){
			cout << "Please enter a letter to guess:";
			cin >> guess;
			cout << endl;
			if (!isalpha(guess[0])){ //if not a letter
				cout << "Invalid guess, please guess again." << endl;
			} else if (guess.length() > 1){ //if more than one letter
				cout << "Invalid guess, please guess again." << endl;
			} else
				valid = true;
		} 
		//send guess
		long sent = sendChar(serverSocket, toupper(guess[0])); 
		if (sent == -1){
			cout << "Error sending guess." << endl;
			return 0;
		}
		
		//recv result
		result = recvLong(serverSocket);
		if (result == -1){
			cout << "Error receiving result." << endl;
			return 0;
		}
		
		//handle result
		if(result == 1)
			cout << "That letter is in the word!" << endl;
		else if (result == 0)
			cout << "That letter is not in the word!" << endl;
		else if (result == 100){
			finished = true;
		} else if (result == -2)
			cout << "Invalid guess."; //incase it isn't checked on client side correctly
	}
	
	//game finished, get end messages
	long finishSize = recvLong(serverSocket);
	if (finishSize == -1){
		cout << "Error receiving finish message size." << endl;
		return 0;
	}
	
	string finishMsg = recvString(serverSocket, finishSize);
	if (finishMsg == "-1"){
		cout << "Error recieving finish message." << endl;
		return 0;
	}
	
	cout << finishMsg;
	
	//get leaderboard
	cout << "Leaderboard: " << endl;
	
	long leaderSize = recvLong(serverSocket);
	if (leaderSize == -1){
		cout << "Error receiving leader board size." << endl;
		return 0;
	}
	
	string leaderBoard = recvString(serverSocket, leaderSize);
	if (leaderBoard == "-1"){
		cout << "Error receiving leader board." << endl;
		return 0;
	}
	
	cout << leaderBoard;
		
			
	close(serverSocket);
		
	
	return 0;
}
