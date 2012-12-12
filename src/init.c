#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>
#include <sys/shm.h>

#include "SDL/SDL.h"
#include "definitions.h"

int socket_id;

///---------TCP SERVER---------///

void addRoom(Room* rooms, SDL_sem** semaphores, int room_num);





/*========== sighandler(signo) ==========

Cleans up after SIGINT has been called.

=========================*/

static void sighandler( int signo ){
  if ( signo == SIGINT ){
    close(socket_id);
    SDL_Quit();
    exit(0);
  }
}

/*========== countSprites(rooms, room_num) ==========
  
Counts the number of initiated sprites in rooms[room_num]
  
=========================*/

int countSprites(Room* rooms, int room_num){
  int i, ans;
  ans = 0;

  //FOR NOW, SKIP THE PLATFORMS PART OF THE COMPONENTS
  for(i=NUMSPRITES;i<NUMTYPES*NUMSPRITES;i++)
    if(rooms[room_num].components[i])
      ans++;
    
  return ans;
}


/*========== subserver(rooms, fd) ==========

Handles a client's inputs and writes the new components to them

=========================*/

void subserver(Room* rooms, SDL_sem** semaphores, int fd, int* rooms_opened){
  struct Message message;
  int curr_room = 0;
  int i, b, temp_curr, numSprites, clientid;
  int keyState[NUMKEYS];
  Uint32 colorkey;

  /*-----------------------------------------
    Initialize the client's player in Room 0 
  -------------------------------------------*/

  Sprite* temp;
  temp = (Sprite*)malloc(sizeof(Sprite));

  read(fd, temp, sizeof(Sprite));
  addSprite( temp, rooms, 0);	
  clientid = temp->clientid;

  
  /*------------------------------------------------------------------------
    Continuously write the room components and handle incoming player inputs
    ------------------------------------------------------------------------*/

  while(1){

    /*------------------------------------
      Write room components to the client
      ------------------------------------*/

    //Notify the client of how many sprites are going to be written
    numSprites = countSprites(rooms, curr_room);

    write(fd, &numSprites, sizeof(int));

    //Close the room components off to prevent collisions
    SDL_SemWait(semaphores[curr_room]);

    //Go through the component array until all initialized sprites have been written
    //FOR NOW, SKIP THE PLATFORMS SEGMENT OF THE COMPONENTS SINCE WE HAVE NONE
    for(i=NUMSPRITES;i<NUMTYPES*NUMSPRITES && numSprites > 0;i++){
      
      temp = rooms[curr_room].components[i];
      
      if(temp)
	b = write(fd, temp, sizeof(Sprite));

      //The write was successful and an initialized sprite was written
      if(b != -1)
	numSprites--;
    }
    
    //Reopen the room components
    SDL_SemPost(semaphores[curr_room]);

    /*------------------
      Read client inputs
      ------------------*/

    //Read the message from client for keystates and an event
    b = read(fd, &message, sizeof(message));

    //The client has closed, so the subserver is no longer needed in this world.
    if(b <= 0){
      puts("Subserver break.");
      break;
    }
    
    for(i=0; i<NUMKEYS; i++)
      read(fd, &(keyState[i]), sizeof(int) );
    

    /*---------------------
      Process client inputs
      ---------------------*/

    temp_curr = curr_room; //Needed in case curr_room changes in serverEventHandler
    SDL_SemWait(semaphores[temp_curr]);

    serverEventHandler(rooms, message, keyState, &curr_room, (*rooms_opened - 1), fd);
    //update all rockets
   
    SDL_SemPost(semaphores[temp_curr]);
  }

  /*-----------------------------------
    Clean up after subserver has closed
    -----------------------------------*/

  removeSprite(rooms, curr_room, clientid);
  exit(0);
}


/*========== main() ==========

Listens for connections and forks off a subserver when it gets one.
Also sets up the rooms that the subservers and clients will use.

=========================*/

int main(){
  SDL_Init(SDL_INIT_EVERYTHING);
  signal(SIGINT, sighandler);

  Room *rooms;
  SDL_sem **semaphores;
  int* rooms_opened; //= (int*)malloc(sizeof(int));
  
  int rooms_id = shmget(68476, sizeof(Room) * 10, 0666 | IPC_CREAT);
  int sem_id = shmget(85938, sizeof(SDL_sem*) * 10, 0666 | IPC_CREAT);

  int rooms_opened_id = shmget(75286, sizeof(int), 0666 | IPC_CREAT);
  //Should have a semaphore for this int

  rooms = (Room*)shmat(rooms_id, NULL, 0);
  semaphores = (SDL_sem**)shmat(sem_id, NULL, 0);
  rooms_opened = (int*)shmat(rooms_opened_id, NULL, 0);

  *rooms_opened = 0;

  int i, b, socket_client;
  char buffer[256];  

  //SELECT variables
  fd_set readers;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;  


  /*------------------
    SERVER PREPARATION
    ------------------*/

  //SOCKET variables
  struct sockaddr_in server;
  socklen_t socket_length;

  //make the server socket for reliable IPv4 traffic 
  socket_id = socket( AF_INET, SOCK_STREAM, 0);
  printf("Socket file descriptor: %d\n", socket_id);

  //set up the server socket struct
  //Use IPv4 
  server.sin_family = AF_INET;

  //This is the server, so it will listen to anything coming into the host computer
  server.sin_addr.s_addr = INADDR_ANY;
  
  //set the port to listen on, htons converts the port number to network format
  server.sin_port = htons(24601);
  
  //bind the socket to the socket struct
  i= bind( socket_id, (struct sockaddr *)&server, sizeof(server) );
  //wait for any connection
  i =  listen( socket_id, 1 );

  /*---------------
    SERVER IS READY
    ---------------*/


  //Add rooms automatically because I always forget to when testing
  addRoom(rooms, semaphores, *rooms_opened);
  *rooms_opened += 1;
  addRoom(rooms, semaphores, *rooms_opened);
  *rooms_opened += 1;


  /*-------------------------------
    ACCEPT CONNECTIONS CONTINUOUSLY
    -------------------------------*/

  while(1) {
    FD_ZERO(&readers);
    FD_SET(STDIN_FILENO, &readers);
    FD_SET(socket_id, &readers);

    printf("Accepting a connection. Options: [add room] [exit]\n");
    select(socket_id + 1, &readers, 0, 0, 0);


    /*-----------
      USER INPUT
      ----------*/

    //The user has ordered us to do something on the server! Oh boy!
    if(FD_ISSET(STDIN_FILENO, &readers)){
      fgets(buffer, sizeof(buffer), stdin);

      //Add room
      if(!strncmp(buffer, "add room", 8)){
	if(*rooms_opened < 10){
	  addRoom(rooms, semaphores, *rooms_opened);
	  *rooms_opened += 1;
	}
	else
	  puts("Maximum number of rooms has been reached.\n");
      }
      
      //Exit
      else if(!strncmp(buffer, "exit", 4)){
	close(socket_id);

	for(i = 0; i < *rooms_opened; i++)
	  SDL_DestroySemaphore(semaphores[i]);

	SDL_Quit();
	break;
      }
    }

    /*-----------------
      SOCKET CONNECTION
      -----------------*/

    //Somebody wants to connect! Alright!
    
    if(FD_ISSET(socket_id, &readers)){      
    
      //set socket_length after the connection is made
      socket_length = sizeof(server); 
      
      //accept the incoming connection, create a new file desciprtor for the socket to the client
      socket_client = accept(socket_id, (struct sockaddr *)&server, &socket_length);
      printf("accepted connection %d\n\n",socket_client);
      
      //Fork off a subserver and close the client off from the main server
      i = fork();
      if ( i == 0 ) {
	subserver(rooms, semaphores, socket_client, rooms_opened);
      }
      else 
	close(socket_client);
      
      
    }
  }

  return 1;
}
  

/*========== addRoom(rooms, room_num) ==========

Sets up a room's components array.

=========================*/

void addRoom(Room* rooms, SDL_sem **semaphores, int room_num){
  
  //Set up the room's component array
  rooms[room_num].components = (Sprite**)calloc(NUMTYPES * NUMSPRITES, sizeof(Sprite*));

  //Create the room's semaphore
  semaphores[room_num] = SDL_CreateSemaphore(1);

  printf("Room %d added.\n\n", room_num);
}


