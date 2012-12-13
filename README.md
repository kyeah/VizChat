VizChat
=======
=======

VizChat is a visual instant messaging tool used to chat and interact with your friends.

Each player has their own avatar and moves to different rooms to chat.

Rooms are essentially arrays of sprites stored on the main TCP server. Player spaces read these components through a subserver, then draw them on the screen.
The player spaces then send events and keystates so that the subserver may update the components.

Messages sent by players are shown to the players in the room and recorded in a chat log visible at the bottom of the screen.
The chat network is implemented using a multicast UDP server forked off of each player space. They will send messages directly to each other.

The avatars themselves can jump and interact with one another. Players can join the game by opening a player space on their computer.

    NOTE: Due to the constraints of SDL 1.2, multiple windows are not possible. 
    Therefore, players can only see the room they are currently in.


------------------------------------

NEEDED:
========

Compiling packages under Ubuntu

    sudo apt-get install build-essential

SDL 1.2 and SDL_ttf

    Open up a package manager and search for SDL.
    Select "libsdl-1.2dev" and "libsdl-ttf2.0-dev"
    Click apply to install.

For more information, please visit http://ubuntu-gamedev.wikispaces.com/How-To+Setup+SDL+for+games+development 

------------------------------------

The Guide to Doing Stuff:
=========================

COMPILING:
    
    Type 'make'. alright!
  
DOING ACTUAL STUFF:
    
    1) Initialize the server with './Server'
    2) Two rooms will be created automatically. You may add as many as ten rooms.
    3) Initialize a player space with './Player <IP Address of server computer>'
    4) If you do not know the IP Address of the server computer, please ask your favorite IT technician relative.
    5) If everything was successful, you have a white screen with one or more characters on it. 

Note: Due to shared memory difficulties, we were not able to get players to see each other. However, the chat system still works, as well as moving between rooms. Collisions theoretically work.
  
--------------------------------------

CONTROLS:

A: Move Left

D: Move Right

SPACE: Jump

W: Jetpack (hopefully)

ENTER: Chat

LEFT MOUSE CLICK: Rockets (THERE BETTER BE ROCKETS)

------------------------------------

Compiled Files:
================

  Player <IP Address> : Initiates a player connection with the server at <IP Address> and opens the window

  Server : Initiates a server to handle rooms and update their components 
    
    (up to 10 rooms supported, with up to 10 of each [platforms, players, rockets] in each room)

------------------------------------

Working Files:
===============

init.c (this is the server file)
    
    - Maintains an array of "rooms" and creates new rooms when asked politely.
    - Updates sprites out of the players' control (platforms, rockets?)

player_space.c
    
    - Creates a new player and listens to input
    - A message is sent out to the server at every pass, holding event and keystate information

input.c
    
    - Handles input
     - ENTER: text bubble and chat log
     - A/D: Move left/right
     - SPACE: Jump
     - W: Jump (or jetpack if time allows)
     - Mouse Release: ROCKETS

    - Handles Collisions
     - Moving to the edge of one room will transition you to the next room

sprite.c
    
    - Handles creation and updating of sprites
    
------------------------------------

Other files:
=============

player.bmp

Teen.ttf

definitions.h 
    
    defines a bunch of macros and structs used in the working files. 
    Also includes the file headers of the working files (except init.c)

Text files for you to read at your leisure
