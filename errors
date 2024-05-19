#include "socket.h"
#include "select.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "playerview.h"
#include "netpacket.h"

// The socket used to communicate with the server. 
Socket theClient;
int playerNum = -1;

// The player's game view.
PlayerView gameView;

void connectToServer(const char *server, int port);
void sendConnectRequest(int player, int sessId);
void handleCtrlC(int sig);

void placeShips();
void playTheGame();
void fireShot();

int main(int argc, char *argv[])
{
  char *server;
  int port;
  int sessId = -1;
  
   /* Set the Ctrl-C signal handler. */
  signal(SIGINT, handleCtrlC);
  
   /* Get the server address and port number from the command-line. */
  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Error: Invalid number of arguments.\n"); 
    fprintf(stderr, "usage:  gameplayer <server-addr> <portnum> [<sess-id>]\n\n");
    return 1;
  }  
  
  server = argv[1];
  port = atoi(argv[2]);
  
  if (argc == 4) 
    sessId = atoi(argv[3]);
  
   /* Connect to the server. */
  connectToServer(server, port);
  
   /* Send the connect request. */
  if (sessId == -1)
    sendConnectRequest(1, 0);
  else
    sendConnectRequest(2, sessId);
  
   /* If player 1, place the ships. */
  if (playerNum == 1) {
    printf("You are player 1.\n");
    placeShips();
  }
  else if (playerNum == 2) {
    printf("You are player 2.\n");
  }
  
   /* Play the game */
  playTheGame();
}

void connectToServer(const char *server, int port)
{
  if (theClient.connect(server, port))
    printf("Connected to server %s on port %d.\n", server, port);
  else {
    fprintf(stderr, "Error: unable to connect to server %s on port %d.\n", server, port);
    exit(1);
  }
}

void sendConnectRequest(int player, int sessId)
{
  ConnRequest request = {0, 0};
  InitResponse response = {0, 0};
  
  request.code = player;
  request.sessionId = sessId;
  
  theClient.send(&request, sizeof(request));
  theClient.recv(&response, sizeof(response));
  
  if (response.code < 0) {
    fprintf(stderr, "Error: invalid connection request.\n");
    exit(1);
  }
  
  playerNum = player;
}

void placeShips()
{
  printf("Place your ships on the grid.\n");
  for (int shipNum = 1; shipNum <= 5; shipNum++) {
    int row, col, dir;
    bool placed = false;
    
    while (!placed) {
      printf("Enter the row, column, and direction (0 for horizontal, 1 for vertical) for ship %d: ", shipNum);
      scanf("%d %d %d", &row, &col, &dir);
      
      PlayerRequest request = {1, row, col, dir, shipNum};
      PlayerResponse response = {0, 0, 0, 0, 0};
      
      theClient.send(&request, sizeof(request));
      theClient.recv(&response, sizeof(response));
      
      if (response.code == 0) {
        gameView.placeShip(row, col, dir, shipNum);
        placed = true;
      } else {
        printf("Error: invalid placement. Try again.\n");
      }
    }
  }
}

void playTheGame()
{
  while (true) {
    fireShot();
  }
}

void fireShot()
{
  int row, col;
  printf("Enter the row and column to fire at: ");
  scanf("%d %d", &row, &col);
  
  PlayerRequest request = {2, row, col, 0, 0};
  PlayerResponse response = {0, 0, 0, 0, 0};
  
  theClient.send(&request, sizeof(request));
  theClient.recv(&response, sizeof(response));
  
  if (response.code == SHOT_MISS) {
    printf("Miss!\n");
    gameView.setMiss(row, col);
  } else if (response.code == SHOT_HIT) {
    printf("Hit!\n");
    gameView.setHit(row, col);
  } else if (response.code == SHOT_SUNK) {
    printf("Hit and sunk!\n");
    gameView.setHit(row, col);
  } else {
    printf("Invalid shot. Try again.\n");
  }
  
  if (response.numShipsPlayer1 == 0 || response.numShipsPlayer2 == 0) {
    printf("Game over. Player %d wins!\n", response.numShipsPlayer1 == 0 ? 2 : 1);
    exit(0);
  }
}

void handleCtrlC(int sig)
{
  printf("\nCtrl-C caught, cleanly exiting.\n");
  theClient.close();
  exit(0);
}
