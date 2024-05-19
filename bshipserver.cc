#include "socket.h"
#include "select.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "netpacket.h"
#include "time.h"

const int DEFAULT_PORT = 30000;

ServerSocket theServer;  
FDSet fdset;
const int MESSAGE_SIZE = 250;
char message[MESSAGE_SIZE+1];

Socket *player1Client = 0;
Socket *player2Client = 0;
int playerSessId = 0;

GameLogic gameLogic;

/* Prototypes. ----------------------------------------------*/
void processRequests();
void handleClientConnection();
void handlePlayerRequest(int, Socket *);
int getSessionID();
void closeConnections();

/* ----------------------------------------------------------*/
int val;

int main(int argc, char *argv[])
{  
   /* Get the port number. */
  int portNum;
  
  if (argc < 2) 
    portNum = DEFAULT_PORT;
  else
    portNum = atoi(argv[1]);
  
   /* Bind the server's listening socket to the given port. */
  if (theServer.bind(portNum)) {
    printf("Server bound to port #%d\n", portNum);
    fdset.addFD(theServer.fd());
  } else {
    printf("Error: can not bind to port #%d\n", portNum);
    exit(1);
  }
  
   /* Initialize the game logic. */
  gameLogic.reset();
  
  /* Gets the random player ID */
  val = getSessionID();
  
   /* Process client requests. */
  processRequests();
}


 /* Process client requests, connections, and messages, after starting the server. */
void processRequests()
{
  while (1) {
     /* Wait for input on the input streams in the set. */
    int *activeSet = fdset.select();
  
     /* Handle input from one of the sockets. */
    int i = 0; 
    while (activeSet[i] >= 0) {
      int fd = activeSet[i];
      i++;
      
      if (fd == theServer.fd()) 
        handleClientConnection();
      else {
        if (player1Client && fd == player1Client->fd())
          handlePlayerRequest(1, player1Client);
        else if (player2Client && fd == player2Client->fd())
          handlePlayerRequest(2, player2Client);
      }
    }    
  }
}

/* Handle a player request, which includes both placing of the ships and 
    taking shots at the opponents' ships. */
void handlePlayerRequest(int player, Socket *clientSocket) {
    PlayerRequest request;
    PlayerResponse response;

    // Read the player request.
    clientSocket->recv(&request, sizeof(request));

    // Process the request based on the operation code.
    switch (request.op) {
    case 1: // Place a ship.
        response.code = gameLogic.placeShip(player, request.row, request.col, request.dir, request.shipType);
        break;

    case 2: // Fire a shot.
        response.code = gameLogic.fireShot(request.row, request.col);
        break;

    default:
        response.code = ERR_INVALID_CODE;
        break;
    }

    // Add missing turn and ship count information to the response.
    response.turn = gameLogic.WhoseTurn;   // Assuming WhoseTurn is public, otherwise use a method to get it.
    response.numShipsPlayer1 = gameLogic.numShips(1);
    response.numShipsPlayer2 = gameLogic.numShips(2);

    // Send the response to the client.
    clientSocket->send(&response, sizeof(response));
}
/* Cleanly closes the connections of both players and resets the game logic. */
void closeConnections()
{
  printf("**A player closed their connection, end the game!\n");
  
  if (player1Client) {
    fdset.removeFD(player1Client->fd());    
    player1Client->close();
    delete player1Client;
    player1Client = 0;
  }
  if (player2Client) {
    fdset.removeFD(player2Client->fd());
    player2Client->close();
    delete player2Client;
    player2Client = 0;    
  }
    
  gameLogic.reset();  
}

/* Functions used with handleClientConnection */
void connectPlayer1(Socket *);
void connectPlayer2(Socket *, int);

/* Manages a client connection as either player 1 or player 2. */ 
void handleClientConnection()
{
   /* Accept the connection. */
  Socket *client = theServer.accept();
  printf("Client connected from: %s on port #%d\n", 
          client->address(), client->port());
  
   /* Read the connection request packet. */
  ConnRequest request;
  InitResponse response = {0, 0};
  
  int result = client->recv(&request, sizeof(request));
  if (result == 0) {
    printf("Connection: client closed the connection before sending request.\n");
    client->close();
    delete client;
  } else {
    if (request.code == 1)
      connectPlayer1(client);
    else if (request.code == 2)
      connectPlayer2(client, request.sessionId);
    else {
      response.code = ERR_INVALID_CODE;
      client->send(&response, sizeof(response));
      client->close();   
      printf("Connection: invalid connection request.\n");
      delete client;      
    }
  }
}

/* Manages the connection for player 1, which must return a session id. */
void connectPlayer1(Socket *client)
{
  InitResponse response = {0, 0};
  if (player1Client) {
    response.code = -1;
    client->send(&response, sizeof(response));
    client->close();
    delete client;
    printf("Error. Invalid number of players.\n");
    exit(1);
  } else {
    player1Client = client;
    fdset.addFD(client->fd());
    
    response.playerSessId = val;
    strcpy(message, "Player 1 is connected.\n");
    printf(message);
    client->send(&response, sizeof(response));
  }
}

/* Manages the connection for player 2, which must verify the session id. */
void connectPlayer2(Socket *client, int sessId)
{
  InitResponse response = {0, 0};
  if (player2Client) {
    response.code = -1;
    client->send(&response, sizeof(response));
    client->close();
    delete client;
    printf("Error. Invalid number of players.\n");
    exit(1);
  } else {
    if (val == sessId) {
      player2Client = client;
      fdset.addFD(client->fd());
     
      strcpy(message, "Player 2 is connected.\n");
      printf(message);
      client->send(&response, sizeof(response));
    } else {
      printf("Invalid session ID.\n");
      response.code = ERR_INVALID_CODE;
      client->send(&response, sizeof(response));
      client->close();
      delete client;
    }
  }
}

int getSessionID()
{
  srand(time(0));
  int val = rand() % 100000 + 1000;
  printf("Sess ID: %d\n", val);
  return val;
}
