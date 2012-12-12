#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define BG 0
#define SPRITES 1
#define ROCKETS 2

#define NUMTYPES 3
#define NUMSPRITES 10
#define NUMKEYS 4

#define DISP_WIDTH 1024
#define DISP_HEIGHT 570

#define TRUE 1
#define FALSE 0

//Multicast variables
#define HELLO_PORT 12345
#define HELLO_GROUP "225.0.0.37"

#define MAXCHATMESSAGES 5
#define MSGLEFT 30
#define MSGTOP 400
#define MSGHEIGHT 20

#define SPACE 0
#define KEY_A 1
#define KEY_D 2
#define KEY_W 3

typedef struct {
  SDL_Surface* sprite; 
  SDL_Rect position;
  int xVelocity;
  int yVelocity;
  int jumpStatus; //When jumping, will increase to the jumpheight before being reset to 0
  int clientid;
  int init;


//for animation
//http://lazyfoo.net/SDL_tutorials/lesson20/index.php
  int frame;
  int status;
//for animation
  /*
  SDL_Rect clips[NUM_ANIMATIONS]; //sprite should be a sprite sheet now, and clips tell you x/y values + the width/height
  When Blitting (Drawing), pass clips[frame] instead of NULL, and it will only use that part of the sprite sheet :)
  */

} Sprite;

typedef struct{
  Sprite** components;
} Room;

struct Message{
  int clientid;
  int roomid;
};


///------------------INPUT-----------------///
void serverEventHandler(Room* rooms, struct Message message, int* keyState, int* curr_room, int last_room_opened, int fd);

void changeRoom(Room* rooms, int last_room_opened, int* curr_room, int next, Sprite* player, int fd);

void serverKeyHandler(int* keyState, Sprite* player);

void handleKeys(int* keyState);

SDL_Surface* handleChat(SDL_Event event, int namelen, char* chatmsg, SDL_Surface *chatsurface, TTF_Font *font, SDL_Color textColor);

int handle_collisions(Sprite* s, Room* rooms, int roomid);

int check_collisions(SDL_Rect A, SDL_Rect B);


///-----------------SPRITE-----------------///
Sprite* loadSprite(char* path, int clientid, int x, int y, Uint32 colorkey);

void drawSprite(Sprite s, SDL_Surface* screen);

void updateSprite(Sprite* s, Room* rooms, int roomid);

void addSprite(Sprite* s, Room* room, int roomNum);

void removeSprite(Room* room, int room_num, int clientid);

///-------------PLAYER_SPACE--------------///

//Chatserver thread
void prepareTextArray();

int chatserver(void *screen);


//Main player_space thread
long unsigned FPScap(long unsigned lastTick);

void run(SDL_Surface* screen, int fd);

void draw(Sprite* components, int numSprites, SDL_Surface* screen);

int server_setup(char* ip_address);

SDL_Surface* initScreen();

void initiate(int fd, Uint32 colorkey);
