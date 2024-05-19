/* gamelogic.cc
 * 
 * version: 1.3  
 * modified: 5/19/20 
 */
 
#include <ctype.h>
#include "gamelogic.h"


const int STATE_SETUP = -1;
const int STATE_PLAY = 1;
const int STATE_END = 0;

/* Store the ships on the grid using cell values of the ship # + 10: 11, 12, 13, 14, 15.
   When a ship is hit, change the number to the negative: -11, -12, -13, -14, -15.
   This allows for quick access to the ship data in the array. 
 */
 
void 
PlayerGrids::reset()
{
  int shipSizes[] = {0, 5, 4, 3, 3, 2};
  
  for(int i = 0; i < 10; i++) 
    for(int j = 0; j < 10; j++) {
      TargetGrid[i][j] = EMPTY_CELL;
      OceanGrid[i][j] = EMPTY_CELL;
    }

  NumShips = 0;  
  for(int i = 1; i <= 5; i++) {
    TheShips[i].row = -1;   // -1 indicates ship not placed.
    TheShips[i].col = -1;
    TheShips[i].code = i + 1;
    TheShips[i].size = shipSizes[i];
  }
}


int
PlayerGrids::placeShip(int row, int col, bool vertical, int shipNum)
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  if(shipNum < 1 || shipNum > 5) throw "Ship number out of range.";
  
  ShipType ship = TheShips[shipNum];
  if(ship.row != -1) 
    return ERR_BADNUM;

  if(vertical) {
    if(row + ship.size - 1 > 9)
      return ERR_BADCELL;
    
    for(int i = row; i < row + ship.size; i++) 
      if(OceanGrid[i][col] != EMPTY_CELL) 
        return ERR_BADCELL;
    
    for(int i = row; i < row + ship.size; i++) 
      OceanGrid[i][col] = shipNum + 10;        
    
  }
  else {
    if(col + ship.size - 1 > 9)
      return ERR_BADCELL;
    
    for(int i = col; i < col + ship.size; i++) 
      if(OceanGrid[row][i] != EMPTY_CELL)
        return ERR_BADCELL;
      
    for(int i = col; i < col + ship.size; i++) 
      OceanGrid[row][i] = shipNum + 10;
 
  }
  
  TheShips[shipNum].row = row;
  TheShips[shipNum].col = col;
  TheShips[shipNum].vertical = vertical;
  
   // Increase the number of floating ships.
  NumShips++;
  
  return 0;
}

int
PlayerGrids::fireShot(int row, int col)
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  
   /* Check the cell. */
  int value = OceanGrid[row][col];
  
   /* Check the shot */
  if(value == EMPTY_CELL) {  // ship missed
    OceanGrid[row][col] = SHOT_MISS;
    return SHOT_MISS;
  }
  else if(value >= 11 && value <= 15) { // ship hit
    OceanGrid[row][col] = -value;       // set ship hit with -ve (ship number+10).
    if(ShipSunk(value - 10)) {
      NumShips--;                // there is one less ship afloat.
      return SHOT_SUNK;
    }
    else
      return SHOT_HIT;     
  }
  else // previously targeted 
    return ERR_BADCELL;
}

void
PlayerGrids::setHit(int row, int col)
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  TargetGrid[row][col] = SHOT_HIT;
}

void 
PlayerGrids::setMiss(int row, int col)
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  TargetGrid[row][col] = SHOT_MISS;  
}

int 
PlayerGrids::getTargetCell(int row, int col) const
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  return TargetGrid[row][col];
}
  
int 
PlayerGrids::getOceanCell(int row, int col) const
{
  if(row < 0 || row > 9 || col < 0 || col > 9) throw "Index out of range.";
  return OceanGrid[row][col];
}

bool
PlayerGrids::ShipSunk(int shipNum) const
{
  if(shipNum < 1 || shipNum > 5) throw "Invalid ship code.";
  
  ShipType ship = TheShips[shipNum];
  if(ship.vertical) {
    for(int i = ship.row; i < ship.row + ship.size; i++)
      if(OceanGrid[i][ship.col] > 0)  // if +ve value, then not hit in that cell.
        return false;            
  }
  else {
    for(int i = ship.col; i < ship.col + ship.size; i++)
      if(OceanGrid[ship.row][i] > 0)  // if +ve value, then not hit in that cell.
        return false;                
  }
  return true;
}


GameLogic::GameLogic()
{
  reset();
}

void 
GameLogic::reset()
{
  GameState = STATE_SETUP;
  WhoseTurn = 1;   
  Winner = 0;
  ThePlayers[0].reset();
  ThePlayers[1].reset();
}


void
GameLogic::startGame()
{
  GameState = STATE_PLAY;
}


int
GameLogic::numShips(int player) const
{
  if(player != 1 && player != 2) throw "Player number out of range.";
  return ThePlayers[player-1].numShips();
}


char *
GameLogic::getTargetCells(int player)
{
  static char TargetCells[100];
    
  if(player != 1 && player != 2) throw "Player number out of range.";
  
  int index = 0;
  for(int i = 0; i < 10; i++) 
    for(int j = 0; j < 10; j++) {
      TargetCells[index] = ThePlayers[player-1].getTargetCell(i, j);
      index++;
    }
   
  return TargetCells;
}


int
GameLogic::fireShot(int row, int col)    
{
  if(GameState != STATE_PLAY) throw "Game not in play mode.";
  
   // Determine the indicies for the player grids. 
  int currentPlayer = WhoseTurn - 1;
  int otherPlayer = !currentPlayer;
  
   // Fire the shot at the opponent's ocean grid.
  int result = ThePlayers[otherPlayer].fireShot(row, col);
  
   // If an error occurs, return the error.
  if(result == ERR_BADCELL) 
    return result;

   // Record the shot in the player's target grid.
  else if(result == SHOT_MISS) 
    ThePlayers[currentPlayer].setMiss(row, col);
  else 
    ThePlayers[currentPlayer].setHit(row, col);
  
   /* If it is player 1's turn, then switch players and continue playing. */
  if(WhoseTurn == 1) {
    WhoseTurn = 2;
    return result;
  }
  else 
    WhoseTurn = 1;
  
   /* If it is player's 2 turn, then see if there is a winner or a draw. */
  if(ThePlayers[0].numShips() == 0) {
    GameState = STATE_END;    
    if(ThePlayers[1].numShips() == 0) 
      Winner = 0;
    else 
      Winner = 1;
  }
  else if(ThePlayers[1].numShips() == 0) {
    GameState = STATE_END;
    Winner = 2;
  }
      
  return result;
}


int
GameLogic::shipSize(int shipNum) const
{
  if(shipNum < 1 || shipNum > 5) throw "Ship number out of range.";
  
  int shipSizes[] = {0, 5, 4, 3, 3, 2};
  return shipSizes[shipNum];  
}


int
GameLogic::placeShip(int player, int row, int col, bool vertical, int shipNum)
{
  if(player != 1 && player != 2) throw "Player number out of range.";
  return ThePlayers[player-1].placeShip(row, col, vertical, shipNum);  
}
