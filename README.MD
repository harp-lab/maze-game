# Maze Game Server
A server written in C++, that creates a maze based on a text file with co-ordinates of walls and sends out the current position of bot and the walls around it, and takes in co-ordinates for the bot to move to. Then captures and visualizes the movements of the bot into a video.

 
![](maze.gif)

### Set-Up
1. Two ways to run the project:
   * Locally after installing all the dependecies
   * In a docker container, using the Dockerfile in the repo, skip to step 4 after starting the container.
2. Dependencies
   * [GraphicsMagick](www.graphicsmagick.org)
   * [FFmpeg](https://ffmpeg.org/)
   * [clang](https://clang.llvm.org/)
   * [boost](https://www.boost.org/)
   * [ghostscript](https://www.ghostscript.com/)
3. After the installing the required dependecies, clone the repo.
4. run `make` to make the ./server executable
5. run `make dfsbot` to create a visualization for a depth first search bot

## Details
-  server can be run with `./server 'program/agent to run'` and communicates with the program passed to it using std-in/out.
-  sends the sense data and bot location over std-in to be read and processed by the agent/program.
-  The agent sends the direction for the bot to move on std-out, which will be read by the server and bot will be moved, updated location of the bot sent back to agent.
-  All the movements of the bot are captured into frames and stitched together into a video.

### Format of communication between server and agent
#### Bot Sense Information
- distance from walls in four directions is passed to std-in in this format
  - `wall up down left right`
  - example: `wall 3.000000 1.000000 3.000000 2.000000`
- updated location of the bot
  - `bot x y`
  - example: `bot 2.014900 1.510483`
#### Agent to server
- direction to move the bot is sent over stdout(printed) in this format
  - `toward x y`
  - example: `toward 1.5 1.5`
### Server Defaults
- Server defaults to the `./mazepool/0.maze`, you can pick another by changing the filename in `./maze-game-server.cpp` or rename the desired file to `0.maze`.
- The number of seconds for the simulation, is set to 99. There is a chance that search wouldn't be complete in those seconds, you can increase|decrease it, but keep it mind, the resource consumption and time to process can change relatively.
- dfsbot.py can act as a template for agents, you can choose to use it as a base for changes, or write one in another language based on it.
- server write the mp4 file to out.mp4, it deletes the out.mp4 at the start of next run.
