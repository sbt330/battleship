#include "socket.h"
#include "select.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "playerview.h"
#include "netpacket.h"

Socket theClient;
PlayerView gameView;
int playerNum = -1;
int sessionID;

void connectToServer(const char *server, int port);
void sendConnectRequest(int playerNum, int sessId);
void handleCtrlC(int sig);
void placeShips();
void playTheGame();

int main(int argc, char *argv[])
{
    char *server;
    int port;

    signal(SIGINT, handleCtrlC);

    if (argc < 3 || argc > 4)
    {
        fprintf(stderr, "Error: Invalid number of arguments.\n");
        fprintf(stderr, "usage:  gameplayer <server-addr> <portnum> [<sess-id>]\n\n");
        return 1;
    }
    else
    {
        server = argv[1];
        port = atoi(argv[2]);
        if (argc == 3)
            playerNum = 1;
        else if (argc == 4)
        {
            sessionID = atoi(argv[3]);
            playerNum = 2;
        }
    }

    connectToServer(server, port);
    sendConnectRequest(playerNum, sessionID);
    placeShips();
    playTheGame();

    return 0;
}

void connectToServer(const char *server, int port)
{
    if (!theClient.connect(server, port))
    {
        printf("Error: can not connect to the server.\n");
        exit(1);
    }

    printf("Connection made.\n");
}

void sendConnectRequest(int playerNum, int sessId)
{
    ConnRequest request = {0};
    InitResponse response;

    request.code = playerNum;
    if (playerNum == 2)
        request.sessionId = sessId;

    theClient.send(&request, sizeof(request));

    int result = theClient.recv(&response, sizeof(response));
    if (result == 0)
    {
        theClient.close();
        exit(0);
    }

    if (response.code == 0)
    {
        printf("Successful connection to server.\n");
    }
    else if (response.code == -1)
    {
        printf("Incorrect number of players.\n");
        exit(0);
    }
    else if (response.code == -2)
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

void placeShips()
{
    PlayerRequest request;
    PlayerResponse response;

    gameView.setMessage("Place ships on your grid.");
    gameView.display();

    int ship = 1;
    while (ship < 6)
    {
        int row, col, dir;
        bool placed = false;
        while (!placed)
        {
            printf("Place the %s.\n", ships[ship]);
            getShipPlacement(ship, &dir, &row, &col);
            row--;
            col--;
            request.op = 1;
            request.row = row;
            request.col = col;
            request.dir = dir;
            request.shipType = ship;

            theClient.send(&request, sizeof(request));
            int result = theClient.recv(&response, sizeof(response));
            if (result == 0)
            {
                printf("Error receiving response from server.\n");
                exit(1);
            }

            if (response.code == 0)
            {
                gameView.setMessage("Ship placed successfully.\n");
                gameView.display();
                placed = true;
            }
            else if (response.code == -11)
            {
                gameView.setMessage("Error: Invalid ship.\n");
                gameView.display();
            }
            else
            {
                gameView.setMessage("Error with placing ships.\n");
                gameView.display();
            }
        }
        ship++;
    }
}

void playTheGame()
{
    PlayerRequest request;
    PlayerResponse response;

    while (true)
    {
        // Player's turn
        gameView.setMessage("Your turn. Take a shot on opponent's grid.");
        gameView.display();
        printf("Enter target coordinates (e.g., A1): ");
        char target[3];
        scanf("%s", target);
        int row = target[0] - 'A';
        int col = target[1] - '1';
        request.op = 2;
        request.row = row;
        request.col = col;
        theClient.send(&request, sizeof(request));

        // Wait for server response
        int result = theClient.recv(&response, sizeof(response));
        if (result == 0)
        {
            printf("Error receiving response from server.\n");
            exit(1);
        }

        if (response.code == 0)
        {
            gameView.setMessage("Shot fired. Waiting for opponent's move.");
            gameView.display();
        }
        else if (response.code == -21)
        {
            gameView.setMessage("Invalid shot. Please try again.");
            gameView.display();
        }
        else if (response.code == -22)
        {
            gameView.setMessage("Shot already fired at this position. Please try again.");
            gameView.display();
        }
        else if (response.code == -23)
        {
            gameView.setMessage("Error processing shot. Please try again.");
            gameView.display();
        }

        // Opponent's turn
        gameView.setMessage("Opponent's turn. Waiting for their move.");
        gameView.display();
        result = theClient.recv(&response, sizeof(response));
        if (result == 0)
        {
            printf("Error receiving response from server.\n");
            exit(1);
        }

        if (response.code == SHOT_HIT)
        {
            gameView.setMessage("Opponent's shot hit your ship at %s. Your turn.");
            gameView.display();
        }
        else if (response.code == SHOT_MISS)
        {
            gameView.setMessage("Opponent's shot missed. Your turn.");
            gameView.display();
        }
        else if (response.code == SHOT_SUNK)
        {
            gameView.setMessage("Opponent's shot sunk your ship. Your turn.");
            gameView.display();
        }

        // Check for game end
        if (response.endGame)
        {
            if (response.winner == playerNum)
            {
                gameView.setMessage("Congratulations! You won the game.");
                gameView.display();
            }
            else
            {
                gameView.setMessage("You lost the game. Better luck next time.");
                gameView.display();
            }
            break;
        }
    }

}

void handleCtrlC(int sig)
{
    if (theClient.isActive())
        theClient.close();
    exit(0);
}

