#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <sys/select.h>
#include <sys/signal.h>

#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#include "SDL/SDL_thread.h"
#include "definitions.h"

char msgbuf[256]; //Shared msgbuf to send messages to the chatserver
SDL_Surface *text[MAXCHATMESSAGES]; //Shared chatlog between the threads (chatserver updates, player_space blits to screen)
int numMessages = 0; //Number of messages in text[]

SDL_Thread *chatserv; 
SDL_sem *msgbuf_sem, *textSem;

char clientname[64];
int namelen;


static void sighandler( int signo ){
  if( signo == SIGINT ){
    SDL_KillThread(chatserv);    
    
    SDL_Quit();
    TTF_Quit();
    
    int i;
    for(i=0; i<numMessages; i++)
      if(text[i] != NULL)
	SDL_FreeSurface(text[i]);
    
    SDL_DestroySemaphore(msgbuf_sem);
    SDL_DestroySemaphore(textSem);
    
    exit(0);
  }
}

    
/*==================================
  
  
         CHATSERVER FUNCTIONS
	 
	 
  ==================================*/


void prepareTextArray(){
  numMessages++;
      
  if(numMessages > 5){

    numMessages--;
    SDL_FreeSurface(text[4]);

  } 
 
  int i;
  for(i = numMessages - 1; i>0; i--)
    text[i] = text[i - 1];

}

  
int chatserver(void *screen){

  signal(SIGINT, sighandler);

  /*-----------------------------------------------------

                  MULTICAST UDP SERVER SETUP

    -----------------------------------------------------*/

  struct sockaddr_in addr, send_addr; //Receiving and sending address 
  int fd, nbytes,addrlen;
  struct ip_mreq mreq;
  //  char msgbuf[MSGBUFSIZE];
  
  u_int yes=1;            /*** MODIFICATION TO ORIGINAL */
  
  /* create what looks like an ordinary UDP socket */
  if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    exit(1);
  }

  /**** MODIFICATION TO ORIGINAL */
  /* allow multiple sockets to use the same PORT number */
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
    perror("Reusing ADDR failed");
    exit(1);
  }
  /*** END OF MODIFICATION TO ORIGINAL */
  
  /* set up receiving address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
  addr.sin_port=htons(HELLO_PORT);
  
  /* set up sending address */
  memset(&send_addr,0,sizeof(send_addr));
  send_addr.sin_family=AF_INET;
  inet_aton("225.0.0.37", &send_addr.sin_addr);
  send_addr.sin_port=htons(HELLO_PORT);

  /* bind to receive address */
  if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
    perror("bind");
    exit(1);
  }
  
  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(HELLO_GROUP);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
    perror("setsockopt");
    exit(1);
  }
  
  /*---------------------------------------------------

                  THE SERVER IS ACTIVE

    ---------------------------------------------------*/




  //Set up font
  TTF_Font *font = TTF_OpenFont("src/Teen.ttf", 16);
  SDL_Color textcolor = {0,0,0}; 

  //Set up select for incoming multicast messages
  fd_set readers;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  
  char last_msgbuf[256];

  /* now just enter a read-print loop */
  while (1) {
    FD_ZERO(&readers);
    FD_SET(fd, &readers);
    
    //Check the multicast socket for any messages (timeout of 0)
    select(fd + 1, &readers, 0, 0, &timeout);
   

    //Check msgbuf to see if the player has entered a message
    SDL_SemWait(msgbuf_sem);

    strcpy(last_msgbuf, msgbuf);
    memset(msgbuf, '\0', 256);

    SDL_SemPost(msgbuf_sem);


    //If msgbuf is not empty, send it to the server
    if(strlen(last_msgbuf) > namelen){  

      sendto(fd, last_msgbuf, sizeof(last_msgbuf), 0, (struct sockaddr *) &send_addr, sizeof(send_addr));
      
    }

    //If there is a message in the socket (including any sent by the player themselves), prepare the message
    if(FD_ISSET(fd, &readers)){
      SDL_SemWait(textSem);

      prepareTextArray();

      addrlen = sizeof(addr);
      recvfrom(fd, last_msgbuf, sizeof(last_msgbuf), 0, (struct sockaddr *) &addr, &addrlen);

      text[0] = TTF_RenderText_Solid(font, last_msgbuf, textcolor);

      SDL_SemPost(textSem);

    }
    
  }

  return 1;
  
}



/*======================================
  
  
       END OF CHATSERVER FUNCTIONS

  
  ======================================*/










/*========== FPScap(lastTick) ==========

Caps the framerate at 60 FPS.

Returns the updated lastTick.
=========================*/

long unsigned FPScap(long unsigned lastTick){
  long unsigned timePassed;
  float loopRate = (1.0 / 60) * 1000;

  timePassed = SDL_GetTicks() - lastTick;
  lastTick = SDL_GetTicks();

  if(timePassed < loopRate){
    SDL_Delay(loopRate - timePassed);
  }

  return lastTick;
}



/*========== run(screen, fd) ==========

1) Reads the components from the server
2) Draws the components
3) Checks for events
4) Sets up the message to be sent
5) Sets up the message's keyStates
6) Sends the message

=========================*/

void run(SDL_Surface* screen, int fd){
  int currRoom, numSprites, i;
  SDL_Event event;

  Sprite components[NUMTYPES*NUMSPRITES];

  //Writing Variables
  struct Message message;
  int keyState[NUMKEYS];
  
  //Room Caption Variables
  char room_caption[7];
  strcpy(room_caption, "Room 0");


  //Local Chat Variables
  int typing = FALSE;  

  TTF_Font *font = TTF_OpenFont("src/Teen.ttf", 16);
  SDL_Color textColor = {0,0,0};

  char chatmsg[256];
  memset(chatmsg, '\0', 256); 

  SDL_Surface *chatsurface = NULL;
  SDL_Rect typePosition;
  typePosition.x = 30;
  typePosition.y = 550;


  //Server-wide chat log display positions
  SDL_Rect position[MAXCHATMESSAGES];

  for(i = 0; i < MAXCHATMESSAGES; i++){
    position[i].x = MSGLEFT;
    position[i].y = MSGTOP + ( MSGHEIGHT * (5 - i) );
  }


  currRoom = 0;
  
  long unsigned lastTick;
  lastTick = SDL_GetTicks();
  
  /*---------------------------------------

             END OF VARIABLE SEA

    ---------------------------------------*/



    while(1){
    //Considering how slow it's going, the FPS cap is probably useless now...
    lastTick = FPScap(lastTick);
    
    //Get the number of sprites to be read
    read(fd, &numSprites, sizeof(int));

    //The room has changed and the new room number has been read instead
    if(numSprites >= 40){
      
      currRoom = numSprites - 40;
      
      //Change room caption
      room_caption[5] = (char)(currRoom + 48);
      SDL_WM_SetCaption(room_caption, NULL);
      printf("Now in %s\n", room_caption);

      //Read number of sprites to be read, for real this time.
      read(fd, &numSprites, sizeof(int));
    
    }

    //Read all sprites
    for(i=0;i<numSprites;i++){
      read(fd, &(components[i]), sizeof(components[i]));
    }    

    //Draw components and blit the current chatsurface if player is typing
    draw(components, numSprites, screen);

    //Draw the chat message in progress
    if(chatsurface && strlen(chatmsg) > 0)
      SDL_BlitSurface(chatsurface, NULL, screen, &typePosition);

    //Draw the chat messages stored in text[]
    SDL_SemWait(textSem);

    for(i = 0; i<numMessages; i++){
      SDL_BlitSurface(text[i], NULL, (SDL_Surface*)screen, &position[i]); 
    }

    SDL_SemPost(textSem);
    
    //Flip the screen
    SDL_Flip(screen);    


    /*---------------
      EVENT HANDLING
      ---------------*/

    SDL_EnableUNICODE( SDL_ENABLE );
    SDL_PollEvent(&event);
    SDL_EnableUNICODE( SDL_DISABLE );


    if(event.type == SDL_QUIT)
      break;
    

    /*-----------------
      ENTER KEY PRESSED
      -----------------*/
    if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN){
      
      //Started typing a message
      if(!typing){ 
	typing = TRUE;
	strcpy(chatmsg, clientname);
	chatsurface = TTF_RenderText_Solid(font, chatmsg, textColor);
      }
      
      //Finished typing a message
      else{
	typing = FALSE;
	
	if(strlen(chatmsg) > namelen){

	  //Write the chat message to the shared msgbuf
	  SDL_SemWait(msgbuf_sem);
	  strcpy(msgbuf, chatmsg);
	  SDL_SemPost(msgbuf_sem);
	  
	  //Reset the chat message
	  memset(chatmsg, '\0', 256);
	}
	
	//Free the chatsurface
	if(chatsurface != NULL){
	  SDL_FreeSurface(chatsurface);
	  chatsurface = NULL;
	}

      }
      
    }

  

    /*----------------
      CONTINUE TYPING
      ----------------*/
    else if(typing){

      //Handle chat if key pressed
      if(event.type == SDL_KEYDOWN)
	chatsurface = handleChat(event, namelen, chatmsg, chatsurface, font, textColor);      

    }
    

    /*--------------
      Message Setup
    ----------------*/
    message.clientid = getpid();
    message.roomid = currRoom;
	     
    for(i=0; i<NUMTYPES; i++)
      keyState[i] = FALSE;

    /*-------------------------
      Check event and keystates
    ---------------------------*/
    if(!typing)
      handleKeys(keyState);

    //Write the clientid and roomid
    write(fd, &message, sizeof(message));

    //Write the keystates
    for(i=0;i<NUMKEYS;i++){
      write(fd, &(keyState[i]), sizeof(int));
    }
   
  }

  if(chatsurface != NULL)
    SDL_FreeSurface(chatsurface);
}


/*========== draw(components, screen) ==========

1) Fills the background with white.
2) Checks every space in the components array and draws it if needed.

=========================*/

void draw(Sprite* components, int numSprites, SDL_Surface* screen){
  
  //Fill the background
  SDL_FillRect( screen, &screen->clip_rect, SDL_MapRGB( screen->format, 255, 255, 255 ) );

  // Draw <numSprites> sprites, to ensure leftover sprites in the component array are not drawn
  int i;
  for(i = 0; i < NUMTYPES*NUMSPRITES && numSprites > 0; i++){
    if(components[i].init == 1){
      drawSprite(components[i], screen);
      numSprites--;
    }
  }
}


/*==========server_setup(ip_address)===========

Sets up the TCP server socket.

===============================*/

int server_setup(char* ip_address){
  int socket_id;
  char buffer[256];
  int i, b;
  
  struct sockaddr_in sock;

  //make the server socket for reliable IPv4 traffic 
  socket_id = socket( AF_INET, SOCK_STREAM, 0);

  printf("Socket file descriptor: %d\n", socket_id);

  //set up the server socket struct
  //Use IPv4 
  sock.sin_family = AF_INET;

  //Client will connect to address in argv[1], need to translate that IP address to binary
  inet_aton( ip_address, &(sock.sin_addr) );
    
  //set the port to listen on, htons converts the port number to network format
  sock.sin_port = htons(24601);

  //connect to the server
  int c = connect(socket_id, (struct sockaddr *)&sock, sizeof(sock));
  printf("Connect returned: %d\n", c);
  
  if(c == -1){
    SDL_Quit();
    puts("Connection Error. Please press CTRL+C to quit fully.");
    exit(0);
  }

  return socket_id;
}


/*========== initScreen() ==========

Initiates the SDL Window and sets the Caption.

Returns the window.
=========================*/

SDL_Surface* initScreen(){
  SDL_Surface* screen;
  SDL_Init(SDL_INIT_EVERYTHING);
  
  screen = SDL_SetVideoMode(DISP_WIDTH, DISP_HEIGHT, 32, SDL_SWSURFACE);
 
  //TODO: Update the caption when room has been changed
  SDL_WM_SetCaption("Room 0", NULL);

  return screen;
}


/*========== initiate(fd, colorkey) ==========

Sends the player sprite to the server

=========================*/

void initiate(int fd, Uint32 colorkey){
  Sprite *temp = loadSprite("src/Untitled.bmp", getpid(), 100, DISP_HEIGHT - 200, colorkey);
  write(fd, temp, sizeof(*temp));
  
}




int main(int argc, char** argv){
  if(!argv[1]){
    puts("No IP input found. Please initiate in the form ' ./Player <Server IP> '");
    exit(1);
  }

  //fgets from stdin to get clientname (and pass that into run, ezpz)
  printf("Please enter your name: ");

  fgets(clientname, 64, stdin);
  clientname[strlen(clientname) - 1] = '\0';

  printf("Hello %s!\n", clientname);

  strcat(clientname, " : ");
  namelen = strlen(clientname);
  ///


  SDL_Surface* screen = initScreen(); 

  //Initiate the semaphores and chatserver thread
  msgbuf_sem = SDL_CreateSemaphore(1);
  textSem = SDL_CreateSemaphore(1);
  chatserv = SDL_CreateThread(chatserver, screen);


  //Initiate the server connection
  int socket_id = server_setup(argv[1]);
    
  
  //Initiate the client's sprite
  Uint32 colorkey = SDL_MapRGB( screen->format, 255, 255, 255);
  initiate(socket_id, colorkey); 



  TTF_Init();
  run(screen, socket_id);


  //CEASE AND DESIST

  SDL_KillThread(chatserv);

  SDL_FreeSurface(screen);
  SDL_Quit();
  TTF_Quit();

  int i;
  for(i=0; i<numMessages; i++)
    if(text[i] != NULL)
      SDL_FreeSurface(text[i]);

  SDL_DestroySemaphore(msgbuf_sem);
  SDL_DestroySemaphore(textSem);

  return 1;
}
