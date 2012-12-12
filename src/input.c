#include <stdlib.h>
#include <stdio.h>

#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include "definitions.h"

/*========== serverEventHandler(rooms, message, keyState) ==========

Updates the sprite using the information held in message and keyState.

=========================*/

void serverEventHandler(Room* rooms, struct Message message, int* keyState, int* curr_room, int last_room_opened, int fd){
  int i;
  Sprite* player;

  //Search the room for the player indicated by the message.clientid
  for(i=0; i < 10; i++){
    player = rooms[message.roomid].components[SPRITES*NUMSPRITES + i];
    
    //Player was found
    if(player && player->clientid == message.clientid){      
      
      //Handle keystates
      serverKeyHandler(keyState, player);

      //Change room if the player has gone to the edge of the screen
      if(player->position.x <= 0)
	changeRoom(rooms, last_room_opened, curr_room, -1, player, fd);

      else if(player->position.x >= DISP_WIDTH - player->position.w)
	changeRoom(rooms, last_room_opened, curr_room, 1, player, fd);
      
      //Update the sprite
      updateSprite(player, rooms, message.roomid);
      break;
    }

  }

}


/*========== changeRoom(rooms, last_room_opened, curr_room, next, player, fd) ==========

Moves the player sprite to a new room.
If next = -1, the player is moving left and moves down a room.
If next = 1, the player is moving right and moves up a room.

last_room_opened is used to wrap the rooms around.

=========================*/

void changeRoom(Room* rooms, int last_room_opened, int* curr_room, int next, Sprite* player, int fd){
  
  if(next == -1)
    player->position.x = DISP_WIDTH - (player->position.w);
  else
    player->position.x = 0;

  //If more than one room initiated...
  if(last_room_opened != 0){

    //If On left edge and in leftmost room
    if(next == -1 && *curr_room == 0){
      addSprite(player, rooms, last_room_opened);
    }  

    //If on right edge and in rightmost room
    else if(next == 1 && *curr_room == last_room_opened){
      addSprite(player, rooms, 0);
    }
    
    //If in a middle room
    else{
      addSprite(player, rooms, *curr_room + next);
    }

    
    //Remove sprite from the old room
    removeSprite(rooms, *curr_room, player->clientid);
    
    //Change the current room counter
    *curr_room += next;

    if(*curr_room < 0)
      *curr_room = last_room_opened;
    else if(*curr_room > last_room_opened)
      *curr_room = 0;

    //Write the new room to the client (add 40 to make sure it is not confused with the number of sprites to be written
    int room_change = *curr_room + 40;
    
    write(fd, &room_change, sizeof(int));
  }
}

/*========== serverKeyHandler(keyState, player) ==========

Updates the player's velocities using keyState.

=========================*/

void serverKeyHandler(int* keyState, Sprite* player){
  player->xVelocity = 0;

  if(keyState[SPACE] && player->yVelocity == 0){
      player->jumpStatus = 1;
      player->yVelocity = -13;
  }

  if(keyState[KEY_A]){
    player->xVelocity += -8;
  }

  if(keyState[KEY_D]){
    player->xVelocity += 8;
  }

  if(keyState[KEY_W]){
  }

}


/*========== handleKeys(keyStates) ==========

Sets up the client's keyStates array to be sent to the server.

=========================*/

void handleKeys(int* keyStates){
  Uint8* keyState;
  keyState = SDL_GetKeyState(NULL);
  
  if(keyState[SDLK_SPACE])
    keyStates[SPACE] = TRUE;
  
  if(keyState[SDLK_a])
    keyStates[KEY_A] = TRUE;
  
  if(keyState[SDLK_d])
    keyStates[KEY_D] = TRUE;
  
  //case SDLK_w:
  
  
  
}

/*========== handleChat(event, namelen, chatmsg, chatsurface, font, textColor) ==========

Handles key inputs when the user is typing a message.

Returns the chat surface created from any changes in the chat message.
=========================*/

SDL_Surface* handleChat(SDL_Event event, int namelen, char* chatmsg, SDL_Surface* chatsurface, TTF_Font *font, SDL_Color textColor){

  SDL_Surface* newSurf = chatsurface;
  int oldLen = strlen(chatmsg);

  //If key = backspace
  if(event.key.keysym.sym == SDLK_BACKSPACE){
    if(oldLen > namelen)
      chatmsg[oldLen - 1] = '\0';
  }  
  //If key = any other
  else if(oldLen < 256){
    chatmsg[oldLen] = (char)event.key.keysym.unicode;
  }
  
  //If chatmsg was changed
  if(strlen(chatmsg) != oldLen){

    //Free old surface
    if(newSurf != NULL)
      SDL_FreeSurface(newSurf);

    //Create new surface from the modified message
    if(strlen(chatmsg) > 0)
      newSurf = TTF_RenderText_Solid(font, chatmsg, textColor);
    else
      newSurf = NULL;
  }

  return newSurf;
}


/*========== handle_collisions(s, rooms, roomid) ==========

Returns TRUE if sprite s is colliding with any other player.

=========================*/

int handle_collisions(Sprite* s, Room* rooms, int roomid){
  int i;
  Sprite* other;

  //ONLY CHECKS PLAYER AGAINST OTHER PLAYERS FOR NOW
  for(i=0; i<NUMSPRITES;i++){
    other = rooms[roomid].components[NUMSPRITES + i];
    

    if(other && //Other sprite is initiated
       other->clientid != s->clientid && //Other sprite is not sprite s
       check_collisions(s->position, other->position) ) // Collisions detected
      return TRUE;
  }
  
  return FALSE;
  
}


/*========== check_collisions(A, B) ==========

Returns TRUE if SDL_Rect A is colliding with SDL_Rect B at any point.

=========================*/

int check_collisions(SDL_Rect A, SDL_Rect B){
  int leftA, leftB; 
  int rightA, rightB; 
  int topA, topB; 
  int bottomA, bottomB;

  //Calculate the sides of rect A 
  leftA = A.x; 
  rightA = A.x + A.w; 
  topA = A.y; 
  bottomA = A.y + A.h; 
  
  //Calculate the sides of rect B 
  leftB = B.x; 
  rightB = B.x + B.w; 
  topB = B.y; 
  bottomB = B.y + B.h;
  
  //Check to see if they collide anywhere
  if( bottomA <= topB ||
      topA >= bottomB ||
      rightA <= leftB ||
      leftA >= rightB )
    return FALSE;  

  //If none of the sides from A are outside B 
  return TRUE;

  
}
