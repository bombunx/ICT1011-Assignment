// #include <TinyScreen.h>
// #include <Wire.h>
// #include <SPI.h>
// #include <GraphicsBuffer.h>
// #include "TinyArcade.h"
#include "RPGSprites.h"

// #define SerialMonitorInterface Serial


// uint8_t defaultFontColor = TS_16b_Black;
// uint8_t defaultFontBG = TS_16b_White;

// const FONT_INFO& font10pt = thinPixel7_10ptFontInfo;


// /*
//   Only use the #define statements for the hardware you are using.
//   Comment out the others to avoid unexpected results.
// */

// //for TinyScreen+
#define moveUpButton display.getButtons(TSButtonUpperLeft)
#define moveDownButton display.getButtons(TSButtonLowerLeft)
#define moveLeftButton display.getButtons(TSButtonLowerRight)
#define moveRightButton display.getButtons(TSButtonUpperRight)

//for TinyArcade
/*
  #define moveUpButton checkJoystick(TAJoystickUp)
  #define moveDownButton checkJoystick(TAJoystickDown)
  #define moveLeftButton checkJoystick(TAJoystickLeft)
  #define moveRightButton checkJoystick(TAJoystickRight)
*/

// TinyScreen display = TinyScreen(TinyScreenDefault);

typedef struct {
  int x; //for x-coordinate on the screen itself (0-95)
  int y; //for y-coordinate on the screen itself (0-63)
  int width; //width of sprite
  int height; //height of sprite
  int bitmapNum; //number used to change bitmaps for animation purposes
  const unsigned int * bitmap; //a pointer to the bitmap currently in use

  int xMap; //for x-coordinate on map grid
  int yMap; //for y-coordinate on map grid
} ts_sprite;

/*
  This constructor initializes the variables inside of the ts_sprite
  object "player." The variables are given values in the order they
  are declared within the ts_sprite structure above. Therefore,
  x = 32, y = 32, width = 16, height = 16, bitmapNum = 0,
  bitmap = playerFaceSouthBMP, xMap = 2, and yMap = 2.
*/
ts_sprite initPlayer = {32, 32, 16, 16, 0, playerFaceSouthBMP, 2, 2};

ts_sprite player = initPlayer;

ts_sprite wallpaper = {16, 16, 16, 16, 0, playerFaceSouthBMP, 1, 1};


//ts_sprite wallpaper = {0, 0, 16, 16, 0, test, 0, 0};

/*
  This array of sprites is used by the drawBuffer() function to display
  each sprite used onto the screen. Note that the background map does
  not need to be included in this array.
*/
int amtSprites = 1;
ts_sprite * spriteList[1] = {
  &player
};

// unsigned long frame = 0; //increments by 1 each time the loop() runs
/*
  FRAME RATE CONTROL is mentioned at the end of the "How to Develop a Game
  for the TinyArcade" tutorial. I have included it here but commented it out.
*/

/*
  Setup simply runs once when the device is booted up to get everything ready.
  In this case, we're just letting the screen know we will be writing to it
  and set some of the properties as needed to prepare the screen.
*/

const uint8_t displayStateGameHome = 0x01;
const uint8_t displayStateGamePlay = 0x02;
const uint8_t displayStateGamePause = 0x03;


uint8_t currentGameDisplayState = displayStateGameHome;

bool rewriteHome = true;
bool rewritePause = true;

bool moving = false;

uint8_t gameStart(bool start){

  bool startGame = start;

  while (startGame){
    if (currentGameDisplayState == displayStateGameHome){
      if (rewriteHome){
        drawDef();
        rewriteHome = false;
        player = initPlayer;
        delay(200);
      }
      if (moveUpButton || moveDownButton || moveLeftButton || moveRightButton){
        display.clearScreen();
        currentGameDisplayState = displayStateGamePlay;
      }
    } else {
      if (currentGameDisplayState == displayStateGamePlay){
        drawBuffer();
        movePlayer();
        if (moveDownButton && moveLeftButton){
          display.clearScreen();
          currentGameDisplayState = displayStateGamePause;
          rewritePause = true;
          delay(500);
        }
      }

      if (currentGameDisplayState == displayStateGamePause){ // Pause Menu 

        if (rewritePause){
          printPause();
          rewritePause = false;
        }

        if (moveDownButton){ // Back Button
          display.clearScreen();
          delay(200); 
          currentGameDisplayState = displayStateGamePlay;
          
        } else if (moveLeftButton){ // Home Button
          display.clearScreen();
          drawDef();
          delay(200);
          currentGameDisplayState = displayStateGameHome;
          rewriteHome = true;

        } else if (moveUpButton){ // Save Button
          display.clearScreen();
          initPlayer = player;
          display.drawRect(0,0,96,64,TSRectangleFilled,TS_16b_White);

          display.setFont(font10pt);
          display.fontColor(TS_16b_Red >> 8, TS_16b_White);
          char saveText[] = "Game Saved!";


          int width=display.getPrintWidth(saveText);
          display.setCursor(48-(width/2), (64 - 10) /2);
          display.print(saveText);
          delay(2000);
          rewritePause = true;
          currentGameDisplayState = displayStateGamePause;

        } else if (moveRightButton){ // Back Button
          display.clearScreen();
          display.drawRect(0,0,96,64,TSRectangleFilled,TS_16b_White);

          display.setFont(font10pt);
          display.fontColor(TS_16b_Red >> 8, TS_16b_White);
          char exitText[] = "Exiting Game . . .";
          char exitText2[] = "Game Exited!";


          int width=display.getPrintWidth(exitText);
          display.setCursor(48-(width/2), (64 - 10) /2);
          display.print(exitText);
          delay(3000);
          display.clearScreen();
          display.drawRect(0,0,96,64,TSRectangleFilled,TS_16b_White);

          width=display.getPrintWidth(exitText2);
          display.setCursor(48-(width/2), (64 - 10) /2);
          display.print(exitText2);
          delay(3000);
          display.clearScreen();

          
          startGame = false;
          rewriteHome = true;
          currentGameDisplayState = displayStateGameHome;
          return 0;          
        }
      }
    }
  }
}


/*
  Background offset is used in writing data to the screen. This has to do
  with how sprites act as they approach the borders of the screen as well
  as how the background moves in relation to the player.
*/
int xBackgroundOffset = 0;
int yBackgroundOffset = 0;

//Global variables used by the drawBuffer() function
const uint16_t ALPHA = 0x1111; //This is just a value we assign for transparency
int backgroundColor = TS_16b_White;

/*
  drawBuffer() is a very involved function. All you really need to know is
  that it works with the sprites array and the background map to produce
  images on the screen by writing and drawing a buffer.
*/
void drawBuffer() {
  uint8_t lineBuffer[96 * 64 * 2];

  display.goTo(0, 0);
  display.startData();
  
  /********************FOR DRAWING BACKGROUND MAP*********************/
  /*
    Note that yTileNum and xTileNum use the current pixel on the screen
    and divide each coordinate by the tile size to figure out which tile
    of the map it needs to write in that location.
  */
  for (int y = 0; y < 64; y++) {
    int yTileNum = (y - yBackgroundOffset) / yTileSize;
    int yTileOffset = (y - yBackgroundOffset) % yTileSize;
    int yBitmapOffset = yTileOffset * yTileSize;

    for (int x = 0; x < 96; x++) {
      int xTileNum = (x - xBackgroundOffset) / xTileSize;
      int xTileOffset = (x - xBackgroundOffset) % xTileSize;

      const unsigned int * bgBitmap = 0;
      byte tile = testMap[yTileNum][xTileNum];
      if (tile == Wall) {
        bgBitmap = wallBMP + yBitmapOffset;
      }
      if (tile == road) {
        bgBitmap = roadBMP + yBitmapOffset;
      }

      unsigned int color = TS_16b_White;
      if (bgBitmap) {
        color = bgBitmap[xTileOffset];
      }
      if(color != ALPHA) {
        lineBuffer[x * 2] = color >> 8;
        lineBuffer[x * 2 + 1] = color;
      }
    }
    
    /***********************FOR DRAWING SPRITES***********************/
    for (int spriteIndex = 0; spriteIndex < amtSprites; spriteIndex++) {
      ts_sprite *cs = spriteList[spriteIndex];
      if (y >= cs->y && y < cs->y + cs->height) {
        int endX = cs->x + cs->width;
        if (cs->x < 96 && endX > 0) {
          int xBitmapOffset = 0;
          int xStart = 0;
          if (cs->x < 0) {
            xBitmapOffset -= cs->x;
          }
          if (cs->x > 0) {
            xStart = cs->x;
          }
          int yBitmapOffset = (y - cs->y) * cs->width;
          for (int x = xStart; x < endX; x++) {
            unsigned int color = cs->bitmap[xBitmapOffset + yBitmapOffset++];
            if (color != ALPHA) {
              lineBuffer[x * 2] = color >> 8;
              lineBuffer[x * 2 + 1] = color;
            }
          }
        }
      }
    }
    display.writeBuffer(lineBuffer, 96 * 2);
  }
  display.endTransfer();
}

//Global variables used by the movePlayer() function
int playerMoveSpeed = 4;
int xTarget = 0;
int yTarget = 0;
uint8_t lastDirection = 0;
  //LAST DIRECTION GUIDE:
  //0 -> South
  //1 -> North
  //2 -> East
  //3 -> West
  
void movePlayer() {
  /*
    Check player location in relation to the desired target. Based on the value
    of the target coordinates and the player's coordinates (screen coordinates,
    not map coordinates) the player will be moved in the respective direction,
    depending on the button input.
  */
  if (yTarget < yBackgroundOffset) { //SOUTH
    if (yBackgroundOffset - yTarget == 8) { //provides animation halfway between tiles
      player.bitmapNum++;
      if (player.bitmapNum > 1) player.bitmapNum = 0;
      if (player.bitmapNum == 0) player.bitmap = playerWalkSouthLeftBMP;
      if (player.bitmapNum == 1) player.bitmap = playerWalkSouthRightBMP;
    }
    yBackgroundOffset -= playerMoveSpeed; //player will move based on set value of playerMoveSpeed
    lastDirection = 0; //Set to a different number for each direction for sprite updating
    //delay(2);
  }
  if (yTarget > yBackgroundOffset) { //NORTH
    if (yTarget - yBackgroundOffset == 8) {
      player.bitmapNum++;
      if (player.bitmapNum > 1) player.bitmapNum = 0;
      if (player.bitmapNum == 0) player.bitmap = playerWalkNorthLeftBMP;
      if (player.bitmapNum == 1) player.bitmap = playerWalkNorthRightBMP;
    }
    yBackgroundOffset += playerMoveSpeed;
    lastDirection = 1;
    //delay(2);
  }
  if (xTarget < xBackgroundOffset) { //EAST
    if (xBackgroundOffset - xTarget == 8) {
      player.bitmap = playerWalkEastBMP;
    }
    xBackgroundOffset -= playerMoveSpeed;
    lastDirection = 2;
    //delay(2);
  }
  if (xTarget > xBackgroundOffset) { //WEST
    if (xTarget - xBackgroundOffset == 8) {
      player.bitmap = playerWalkWestBMP;
    }
    xBackgroundOffset += playerMoveSpeed;
    lastDirection = 3;
    //delay(2);
  }
/***********************************************************************************/
  if (xTarget == xBackgroundOffset && yTarget == yBackgroundOffset) { //Checks if player is at target coordinate
    moving = false; //If the target is reached, moving is set to false

    //Bitmap is updated depending on the last button press
    if (lastDirection == 0) player.bitmap = playerFaceSouthBMP;
    if (lastDirection == 1) player.bitmap = playerFaceNorthBMP;
    if (lastDirection == 2) player.bitmap = playerFaceEastBMP;
    if (lastDirection == 3) player.bitmap = playerFaceWestBMP;

    //Player's map coordinates are updated to keep track of where they are
    player.yMap = (player.y - yBackgroundOffset) / yTileSize;
    player.xMap = (player.x - xBackgroundOffset) / xTileSize;
  }
/***********************************************************************************/
  
/*
  Checks for user input in the specified direction and makes sure that the
  adjacent tile in that direction is a tile that can be walked onto and not a
  wall. Here, we specifically check that the tile has an ID humber below 70.
  This is an arbitrary number chosen such that all tiles 70 and above can not
  be walked onto and all below 70 are able to be walked across. A similar system
  can be implemented for doors and warp tiles.
*/
  if (testMap[player.yMap - 1][player.xMap] != 70 && moveUpButton && !moving) { //NORTH
    moving = true;
    yTarget += 16;
    player.bitmap = playerFaceNorthBMP;
  }
  if (testMap[player.yMap + 1][player.xMap] != 70 && moveDownButton && !moving) { //SOUTH
    moving = true;
    yTarget -= 16;
    player.bitmap = playerFaceSouthBMP;
  }
  if (testMap[player.yMap][player.xMap - 1] != 70 && moveLeftButton && !moving) { //EAST
    moving = true;
    xTarget += 16;
    player.bitmap = playerFaceWestBMP;
  }
  if (testMap[player.yMap][player.xMap + 1] != 70 && moveRightButton && !moving) { //WEST
    moving = true;
    xTarget -= 16;
    player.bitmap = playerFaceEastBMP;
  }
/***********************************************************************************/

/*
  These lines can be implemented for troubleshooting and making sure that
  the player's coordinates on the map are correct.
*/
  SerialUSB.print("X: "); SerialUSB.println(player.xMap);
  SerialUSB.print("Y: "); SerialUSB.println(player.yMap);
}

int count = 0;

void drawDef() {
  uint8_t lineBuffer[96 * 64 * 2];
  for (int i =0; i < 96 * 64 * 2; i++){
    lineBuffer[i] = TS_16b_White;
  }
  
  //display.drawRect(0,0,96,64,TSRectangleFilled,ALPHA);
  display.goTo(0, 0);
  display.startData();

  for (int y = 0; y < 64; y++) {
    
    if (y >= wallpaper.y && y < wallpaper.y + wallpaper.height) {
      int endX = wallpaper.x + wallpaper.width;
      if (wallpaper.x < 96 && endX > 0) {
        int xBitmapOffset = 0;
        int xStart = 0;
        if (wallpaper.x < 0) {
          xBitmapOffset -= wallpaper.x;
        }
        if (wallpaper.x > 0) {
          xStart = wallpaper.x;
        }
        int yBitmapOffset = (y - wallpaper.y) * wallpaper.width;
        for (int x = xStart; x < endX; x++) {
          unsigned int color = wallpaper.bitmap[xBitmapOffset + yBitmapOffset++];
          if (color != ALPHA) {
            lineBuffer[x * 2] = color >> 8;
            lineBuffer[x * 2 + 1] = color;
          }
          else {
            lineBuffer[x * 2] = TS_16b_White >> 8;
            lineBuffer[x * 2 + 1] = TS_16b_White >> 8;
          }
        }
      }
    } else {
      for (int x = wallpaper.x; x < wallpaper.x + wallpaper.width; x++) {
        lineBuffer[x * 2] = TS_16b_White >> 8;
        lineBuffer[x * 2 + 1] = TS_16b_White;
      }
    }
    display.writeBuffer(lineBuffer, 96 * 2);
  }
  display.endTransfer();


  display.setFont(font10pt);
  display.fontColor(TS_16b_Red >> 8, TS_16b_White);


  display.setCursor(1, 52);
  char testStr[] = "Start";
  display.print(testStr);
  // count++;
  // display.clearScreen();
  // display.setFont(font10pt);
  // display.fontColor(defaultFontColor, defaultFontBG);
  // display.setCursor(1, 52);
  // display.print(F("PRESS"));
}

void printPause(){
  uint8_t menuTextY[2] = {1, 54};
  
  display.drawRect(0,0,96,64,TSRectangleFilled,backgroundColor);

  display.setFont(font10pt);
  display.fontColor(TS_16b_Red >> 8, TS_16b_White);

  char pause[] = "P A U S E ";
  char pauseUp[] = "< SAVE";
  char pauseDown[] = "< BACK";
  char pauseLeft[] = "HOME >";
  char pauseRight[] = "EXIT >";


  int width=display.getPrintWidth(pause);
  display.setCursor(48-(width/2), (64 - 10) /2);
  display.print(pause);

  width = display.getPrintWidth(pauseUp);
  display.setCursor(0, menuTextY[0]);
  display.print(pauseUp);

  width = display.getPrintWidth(pauseDown);
  display.setCursor(0, menuTextY[1]);
  display.print(pauseDown);

  width = display.getPrintWidth(pauseLeft);
  display.setCursor(96 - 1 - width, menuTextY[1]);
  display.print(pauseLeft);

  width = display.getPrintWidth(pauseRight);
  display.setCursor(96 - 1 - width, menuTextY[0]);
  display.print(pauseRight);

}