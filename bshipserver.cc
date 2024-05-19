#include "socket.h"
#include "select.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "netpacket.h"
#include "time.h"

const int DEFAULT_PORT = 30000;

ServerSocket theServer;
FDSet fdset;
const int MESSAGE_SIZE = 250;
char message[MESSAGE_SIZE + 1];
Socket *player1Client = 0;
Socket *player2Client = 0;
int playerSessId = 0;

GameLogic gameLogic;

void processRequests();
void handleClientConnection();
void handlePlayerRequest(int, Socket *);
int getSessionID();
void closeConnections();

int main(int argc, char *argv[])
{
    int portNum;

    if (argc < 2)
        portNum = DEFAULT_PORT;
    else
        portNum = atoi(argv[1]);

    if (theServer.bind(portNum))
    {
        printf("Server bound to port #%d\n", portNum);
        fdset.addFD(theServer.fd());
    }
    else
    {
        printf("Error: can not bind to port #%d\n", portNum);
        exit(1);
    }

    gameLogic.reset();
    int val = getSessionID();

   





