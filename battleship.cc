/* gameplayer.cc
 *
 * This program is the player client part of the network version of the game 
 * of battleship. The ip address (or host name) and listening port 
 * number of the server is provided on the command-line. Player 2 must also
 * provide the game session id. 
 * 
 *   gameplayer <server-addr> <portnum> [<sess-id>]
 *
 * - After the game ends, the client pauses until the user presses Ctrl-C to 
 *   terminate the program.
 */

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


int main(int argc, char *argv[])
{
  char *server;
  int port;
  int sessId = -1;
  
   /* Set the Ctrl-C signal handler. */
  signal(SIGINT, handleCtrlC);
  
   /* Get the server address and port number from the command-line. */
  if( argc < 3 || argc > 4) {
    fprintf(stderr, "Error: Invalid number of arguments.\n" ); 
    fprintf(stderr, "usage:  gameplayer <server-addr> <portnum> [<sess-id>]\n\n" );
    return( 1 );
  }
  else {
    server = argv[1];
    port = atoi(argv[2]);
    if(argc == 3)
      playerNum = 1;
    else if(argc == 4) {
      sessId = atoi(argv[3]);
      playerNum = 2;
    }
  }
  
   /* Connect to the server using the given address and port number. */
  connectToServer(server, port);
  
   /* Send the connection request and process the response. */
  sendConnectRequest(playerNum, sessId);
  
   /* Place ships. */
  placeShips();
  
   /* Play the game. */
  playTheGame();
}


 /* Play the actual game after the ships have been placed. */
void playTheGame()
{
  int result;
  char response[256];
  char request[256];
  
  gameView.setMessage("Player 1, fire first shot.\n");
  gameView.display();
 
  while (1)
  {
  	  //idk
  	  
  	  
  }
  
  
  
}


void sendConnectRequest(int playerNum, int sessId) 
{
  int result;
  ConnRequest request = {0, 0};
  InitResponse response;
  
   /* Build and send the connection request. */   
  request.code = playerNum;  
  if(playerNum == 2) 
    request.sessionId = sessId;
    
  theClient.send(&request, sizeof(request));
    
   /* Wait for the response. */
  result = theClient.recv(&response, sizeof(response));
  if( result == 0) {
    theClient.close();
    exit(0);
  }
          
   /* Handle the response. */
   
   if (response.code == 0)
   {
   	   printf("Successful connection to server.\n");
   }
   else if (response.code == -1)
   {
   	   printf("Incorrect number of players.\n");
   	   exit(0);
   }
   else if(response.code == -2)
   {
   	   printf("Invalid session ID. \n");
   	   exit(0);
   }
   else if (response.code == -3)
   {
   	   printf("All players have not yet connected.\n");
   	   exit(0);
   }
   else
   {
   	   printf("Unknown error. Response code: %d\n", response.code);
   	 
   }
   
}


 /* Handles the user interaction and server communications to place the
    player's ships on the grid. */
void placeShips()
{
  void getShipPlacement(int ship, int *dir, int *row, int *col);  
  
  PlayerRequest request;
  PlayerResponse response;
  
  gameView.setMessage("Place ships on your grid.");
  gameView.display();
  
  int row, col;
  int dir;
  
  int ship = 1;
  while(ship < 6) {
    getShipPlacement(ship, &dir, &row, &col);
    row--;
    col--;
    
    /* Send the placement to the server. */
    request.op = 1;
    request.row = row;
    request.col = col;
    request.dir = dir;
    request.shipType = ship;
    theClient.send(&request, sizeof(request));
    printf("Send placement for ship %d to server\n", ship);
    
     /* Wait for the response. */
    int result = theClient.recv(&response, sizeof(response));
    printf("Recieved response from ship %d: result=%d, code=%d\n", ship, result, response.code);
    if(result == 0) 
    {
    	printf("Error receiving response from server.\n");
    	exit(1);
    }
    
    
    /* Handle the response. */
    printf("Response code: %d\n", response.code);
    if (response.code == 0)
    {
    	 gameView.setMessage("Ship placed successfully.\n");
    	 ship++;
    }
    else if (response.code == -11)
    {
    	 gameView.setMessage("Error: Invalid ship.\n");
    }
    else
    {
    	 gameView.setMessage("Error with placing ships.\n");
    }
   
  
  }  
  
  gameView.setMessage("Waiting for the game to start.");
  gameView.display();
}


 /* Handle user interaction to obtain the placement of one ship. */
void getShipPlacement(int ship, int *dir, int *row, int *col) 
{  
  const char *ships[] = { 0, 
      "Carrier (5)",
      "Battleship (4)",
      "Cruiser (3)",
      "Submarine (3)",
      "Destroyer (2)"
  };
  
  bool ok = false;
  char direction[100];
  
  while(!ok) {      
    printf("Place the %s.\n", ships[ship]);
      
    ok = false;
    printf("  Direction (h)orizontal or (v)ertical: ");
    scanf("%s", direction);
    
    if(direction[0] == 'h' || direction[0] == 'v') {
      if(direction[0] == 'v') 
        *dir = 1;
      else 
        *dir = 0;
      
      printf("  Row (1-10): ");
      scanf("%d", row);
      if(*row < 1 || *row > 10) 
        printf("Error: Invalid row value.\n\n");
      else {
        printf("  Column (1-10): ");
        scanf("%d", col);
        if(*col < 1 || *col > 10)
          printf("Error: Invalid column value.\n\n");
        else 
          ok = true;       
      }
    }
    else 
      printf("Error: Invalid direction.\n\n");
  }
}


 /* Connect to the battleship server. */
void connectToServer(const char *server, int port)
{
   /* Connect to the server at the given port. */
  if(!theClient.connect(server, port)) {
    printf("Error: can not connect to the server.\n");
    exit(1);
  }
  
   /* Connection made. Send the connection request. */
  printf( "Connection made.\n");
}


 /* Action to take when Ctrl-C is pressed. */
void handleCtrlC(int sig)
{
  if(theClient.isActive())
    theClient.close();
  exit(0);  
}
