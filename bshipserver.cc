#include "socket.h"
#include "select.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "netpacket.h"
#include "time.h"

const int DEFAULT_PORT = 30000;
const int MESSAGE_SIZE = 250;

ServerSocket theServer;
FDSet fdset;
char message[MESSAGE_SIZE + 1];
Socket *player1Client = nullptr;
Socket *player2Client = nullptr;
int playerSessId = 0;

GameLogic gameLogic;

void processRequests();
void handleClientConnection();
void handlePlayerRequest(int fd, Socket *clientSocket);
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
    playerSessId = getSessionID();

    // Main loop to handle client connections and requests
    while (true)
    {
        fdset.set();

        if (select(fdset.maxFD() + 1, fdset.getFDSet(), NULL, NULL, NULL) < 0)
        {
            perror("Error in select");
            closeConnections();
            exit(EXIT_FAILURE);
        }

        if (fdset.isSet(theServer.fd()))
        {
            handleClientConnection();
        }

        processRequests();
    }

    return 0;
}

void processRequests()
{
    if (player1Client != nullptr)
    {
        handlePlayerRequest(player1Client->fd(), player1Client);
    }

    if (player2Client != nullptr)
    {
        handlePlayerRequest(player2Client->fd(), player2Client);
    }
}

void handleClientConnection()
{
    Socket *clientSocket = theServer.accept();
    if (clientSocket == nullptr)
    {
        perror("Error accepting client connection");
        closeConnections();
        exit(EXIT_FAILURE);
    }

    int sessionId = clientSocket->recvInt();
    if (sessionId != playerSessId)
    {
        printf("Invalid session ID. Connection rejected.\n");
        delete clientSocket;
        return;
    }

    printf("Player connected with session ID: %d\n", sessionId);

    if (player1Client == nullptr)
    {
        player1Client = clientSocket;
    }
    else if (player2Client == nullptr)
    {
        player2Client = clientSocket;
    }

    fdset.addFD(clientSocket->fd());
}

void handlePlayerRequest(int fd, Socket *clientSocket)
{
    // Handle player's requests here
}

int getSessionID()
{
    srand(time(NULL));
    return rand() % 100000 + 1;
}

void closeConnections()
{
    if (player1Client != nullptr)
    {
        player1Client->closeSocket();
        delete player1Client;
    }

    if (player2Client != nullptr)
    {
        player2Client->closeSocket();
        delete player2Client;
    }

    theServer.closeSocket();
}


   





