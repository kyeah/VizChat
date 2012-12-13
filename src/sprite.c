#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "SDL/SDL.h"
#include "definitions.h"


/*======== loadSprite(path, clientid, x, y, colorkey) ==========

Loads the bitmap indicated by path into a struct Sprite. 
The colorkey is applied, and the Sprite is initialized at (x,y) with a gravitational yVelocity of 2.

Returns the Sprite.
====================*/

Sprite* loadSprite(char* path, int clientid, int x, int y, Uint32 colorkey){
  SDL_Init(SDL_INIT_EVERYTHING);
  Sprite* s = (Sprite*)malloc(sizeof(Sprite));
  SDL_Rect offset;

  //Temporary SDL_Rect for the sprite's position
  offset.x = x;
  offset.y = y;
  
  //HARDCODE RANDOM STUFF FOR NOW UNTIL I FIGURE OUT HOW TO GET WIDTH AND HEIGHT OF A BITMAP
  offset.w = 50;
  offset.h = 100;

  //Load and optimize the image referred to by path
  SDL_Surface *temp, *optimizedImg;
  temp = SDL_LoadBMP(path);
  optimizedImg = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);

  //Set the color that is to be transparent on the sprite bitmap
  SDL_SetColorKey(optimizedImg, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorkey);
  
  //Set the sprite's variables
  s->sprite = optimizedImg;
  s->clientid = clientid;
  s->position = offset;
  s->xVelocity = 0;
  s->jumpStatus = 0;
  s->init = TRUE;  

  //Set gravity or keep platform stationary
  if(!strncmp(path, "platform.bmp", 12))
    s->yVelocity = 0;
  else 
    s->yVelocity = 13;
  
  

  return s;
}
  

/*======== drawSprite(s, screen) ==========

Draws the sprite to the screen.

====================*/

void drawSprite(Sprite s, SDL_Surface* screen){
  SDL_BlitSurface(s.sprite, NULL, screen, &(s.position) );

  /*Once clips are in place, should be replaced by...
    if(s.dir == LEFT)
    SDL_BlitSurface(s.sprite, &(s.leftclip[s.frame]), screen, &(s.position));
    else if(s.dir == RIGHT)
    SDL_BlitSurface(s.sprite, &(s.rightclip[s.frame]), screen, &(s.position));
  */
}


/*======== updateSprite(s) ==========

Updates the sprite's jump status and position.

====================*/

void updateSprite(Sprite* s, Room* rooms, int roomid){
  //FIRST HALF OF JUMPING
  if(s->jumpStatus > 0){
    s->jumpStatus += 1;

  }
  
  //AT APEX OF JUMP
  if(s->jumpStatus >= 10){
    s->jumpStatus = 0;
    s->yVelocity = 13;
  }
  
  //HIT THE GROUND
  if(s->position.y >= DISP_HEIGHT - s->position.h && s->yVelocity > 0){
    s->yVelocity = 0;
  }

  //SHOULD HANDLE ANIMATION HERE
  //in handle_anim(s), if(yVelocity != 0) set frame to JUMP
  //else if(xVelocity < 0) set dir LEFT and frame++
  //else if(xVelocity > 0) set dir RIGHT and frame++
  //else if(xVelocity == 0) set frame to 0

  //Update Position
  s->position.x += s->xVelocity;
  s->position.y += s->yVelocity;

  //If the position invades another sprite's space, move it back to its original position.
  if(handle_collisions(s, rooms, roomid)){
    s->position.x -= s->xVelocity;
    s->position.y -= s->yVelocity;
  }

}

/* Comment out for now until it's in working order :)
void handle_anims(*Sprite s){
	
	int yV = s->yVelocity
	
	//need to set a jumpframe corresponding to the clip in clips for jumping
	int jumpFrame;
	
	if(yV != 0){
	  s->frame = s->clips[jumpFrame];
	  
	  else if(s->xVelocity < 0){
	    s->dir = LEFT;
	    s->frame++;
	    
	  }
	}
}
*/

/*======== addSprite(s, components) ==========

Adds the sprite to the first available index of the room components.
-
====================*/

void addSprite(Sprite* s, Room* room, int roomNum){
  int i;
  
  for(i=0;i<NUMSPRITES;i++){
    if(!room[roomNum].components[SPRITES*NUMSPRITES + i]){
      room[roomNum].components[SPRITES*NUMSPRITES + i] = s;
      break;
    }
  }

}


/*======== removeSprite(s, components) ==========

Removes the sprite indicated by clientid from room[room_num]

====================*/

void removeSprite(Room* room, int room_num, int clientid){
  int i;

  for(i=0;i<NUMSPRITES;i++){
    if(room[room_num].components[NUMSPRITES + i]->clientid == clientid){
      room[room_num].components[NUMSPRITES + i] = NULL;
      break;
    }
  }
}
