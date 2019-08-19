/* Diego Carregha
   Computing Systems
   Final Project
   game_server.cpp
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
#include <fstream>  
#include <stdlib.h>
#include <stdio.h>
#include <time.h> 
#include "P4.h"

using namespace std;

//leader board struct
struct leaders{
    string username = "null";
    float score = 0;
	string scoreSt;
} leaders[3];

//thread arguments
struct ThreadArgs{
    int clientSocket;
};

pthread_mutex_t mutex;

void *threadGame(void *args);

int main(int argc, char **argv)
{
    // Check args
    if (argc != 2){
		cout << "Not enough arguments, please enter port." << endl;
		return 0;
	}

    //define and check port
    short portNum = stoi(argv[1]);
    if (portNum > 16499 || portNum < 16450){
        cout << "Invalid port." << endl;
        return 0;
    }

    //create socket
    int socketNum = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketNum == -1){
        cout << "Error creating socket." << endl;
		return 0;
	}

    //server setup
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(portNum);

    //bind port to socket
    int status = bind(socketNum, (sockaddr *)(&serverAddress), sizeof(serverAddress));
    if (status != 0){
        cout << "Error binding." << endl;
        return 0;
    }

    //check for new connections
    status = listen(socketNum, 5);
    if (status == -1){
		cout << "Error listening." << endl;
		return 0;
    }
	

    ThreadArgs *threadArgs;

	while (true){
        cout << "Listening..." << endl; 
		//will send while waiting for client connection and username

		//connect to client
        sockaddr_in clientAddr;
        socklen_t addressLength = sizeof(clientAddr);
        int clientNum = accept(socketNum, (sockaddr *)(&clientAddr), &addressLength);
        if (clientNum == -1){
            cout << "Client failed to connect." << endl;
            return 0;
        } else {
            cout << "Client connected." << endl;
        }

		//game thread
        threadArgs = new ThreadArgs;
        threadArgs->clientSocket = clientNum;

        pthread_t threadID;
        status = pthread_create(&threadID, NULL, threadGame, (void *)(threadArgs));
        if (status != 0){
            cout << "Error creating pthread." << endl;
            return 0;
        }
    }
    return 0;
}

void *threadGame(void *args)
{
	struct ThreadArgs *threadArgs = (ThreadArgs *)args;
    int clientSocket = threadArgs->clientSocket;
    delete threadArgs;
	
	//get rand word
	string line;
	string fileName;
	
	//open words file
	fileName = "/home/fac/lillethd/cpsc3500/projects/p4/words.txt";
	ifstream infile;
	infile.open(fileName);
	
	//generate rand num
	srand(time(NULL));
	int randNum = rand() % 57488;
	
	//read lines until at rand index
	for (int i = 0; i <= randNum; i++){
		getline(infile, line);
	}
	
	cout << "Random word: " << line << endl;
	
	//get client name (size first then name)
    long nameSize = recvLong(clientSocket);
    if (nameSize == -1){
        pthread_detach(pthread_self());
        close(clientSocket);
        return NULL;
    }
    string username = recvString(clientSocket, nameSize);
    if (username == "-1"){
        pthread_detach(pthread_self());
        close(clientSocket);
        return NULL;
    }
	
	long turns = 0;
	long wordLength; //length of rand word
	string wordPlay; //for displaying to client
	bool finished = false;
	int result = 0;
	
	wordLength = line.length();
	for (int i = 0; i < wordLength; i++){
		wordPlay += '-';
	}
	wordPlay += '\0';
	
	char guessed[wordLength]; //saves what has been guessed for comparing
	strcpy(guessed, wordPlay.c_str());
	
	while (!finished){
		turns++;
		
		//send turns (should start at 1)
		long sendTurns = sendLong(clientSocket, turns);
		if (sendTurns == -1){
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		
		//send wordLength and wordPlay
		long sendLength = sendLong(clientSocket, wordLength + 1);
		if (sendLength == -1){
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		
		long sendWord = sendString(clientSocket, wordPlay);
		if (sendWord == -1){
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}


		//receive guess
		string guess = recvString(clientSocket, 1);
		if (guess == "-1"){
			pthread_detach(pthread_self());
			close(clientSocket);
			return NULL;
		}
		
		//make sure is alpha
		if (isalpha(guess[0])){
			guess = toupper(guess[0]);
			
			result = 0; //initially
			//check if in word
			for (int i = 0; i < wordLength; i++){
				if (line[i] == guess[0]){
					guessed[i] = guess[0];
					result = 1; //change if found in word
				}
			}
			
			wordPlay = guessed[0]; //copy changes
			for (int i = 1; i < wordLength; i++){
				wordPlay += guessed[i];
			}
			wordPlay += '\0';
			
			int same = 0; //check if guessed and the chosen word are the same
			for (int i = 0; i < wordLength; i++){
				if (line[i] == guessed[i])
					same++;
			}
			if (same == wordLength){
				result = 100;
				finished = true;
			}
		} else //if the guess wasn't alpha, make the user re-enter guess
			result = -2;
		
		//send result
		long sendResult = sendLong(clientSocket, result);
		if (sendResult == -1){
			pthread_detach(pthread_self());
			close(clientSocket);
		}
	}
	//game finished, send finish msg, calc score
	float score = (float(turns)/float(wordLength));
	char scoreSt[4];
	sprintf(scoreSt, "%3.2f", score);
	
	string finishMsg = "Congratulations! You guessed the word: " + line + " in " + 
	to_string(turns) + " turns. Your score is: " + scoreSt + ". \n\0";
	
	long finishLength = finishMsg.length();
	
	//send finishLength
	long sendFin = sendLong(clientSocket, finishLength);
	if (sendFin == -1){
		pthread_detach(pthread_self());
		close(clientSocket);
	}
	//send finishMsg
	long sendMsg = sendString(clientSocket, finishMsg);
	if (sendMsg == -1){
		pthread_detach(pthread_self());
		close(clientSocket);
	}
	
	//update leaders
	for (int i = 0; i < 3; i++){
        if (leaders[i].score == 0 || score < leaders[i].score){
            pthread_mutex_lock(&mutex);
			if (i == 0) { //first place
				for (int j = 2; j > 0; j--){
					leaders[j].username = leaders[j-1].username;
					leaders[j].score = leaders[j-1].score;
					leaders[j].scoreSt = leaders[j-1].scoreSt;
				}
				leaders[i].username = username;
				leaders[i].score = score;
				leaders[i].scoreSt = scoreSt;
				i = 3;
			}
			else if (i == 1){ //second
				leaders[i+1].username = leaders[i].username;
				leaders[i+1].score = leaders[i].score;
				leaders[i+1].scoreSt = leaders[i].scoreSt;
				leaders[i].username = username;
				leaders[i].score = score;
				leaders[i].scoreSt = scoreSt;
				i = 3;
			}
			else if (i == 2){//third
				leaders[i].username = username;
				leaders[i].score = score;
				leaders[i].scoreSt  = scoreSt;
				i = 3;
			}		
            pthread_mutex_unlock(&mutex);
        }
    }
	
	//send leader board
	string leaderBoard;
	for (int i = 0; i < 3; i++){
		if (leaders[i].score != 0){
			leaderBoard += to_string(i + 1) + ": " + leaders[i].username + ' ' +
			leaders[i].scoreSt + "\n";
		}
	}
	leaderBoard += '\0';
	
	long leadSize = leaderBoard.length();
	long leadSizeSend = sendLong(clientSocket, leadSize);
	if (leadSizeSend == -1){
		pthread_detach(pthread_self());
		close(clientSocket);
	}
	
	long leadMsgSend = sendString(clientSocket, leaderBoard);
	if (leadMsgSend == -1){
		pthread_detach(pthread_self());
		close(clientSocket);
	}
	
	pthread_detach(pthread_self());
	close(clientSocket);
	
	return NULL;
}
	