# Bombing Robots

My solution for the second project (the big one) of the [Computer networks (pol. Sieci komputerowe)](https://usosweb.mimuw.edu.pl/kontroler.php?_action=katalog2/przedmioty/pokazPrzedmiot&prz_kod=1000-214bSIK) course taken in the 2021/2022 summer semester.

## Task

The task was to implement a network multiplayer game - simplified version of [Bomberman](https://en.wikipedia.org/wiki/Bomberman).

### Full description in polish

[**here**](https://github.com/agluszak/mimuw-sik-2022-public/blob/master/README.md)

## Game

### Rules

The game is played on a rectangular screen. At least one player takes part in it. Each player controls the robot's movement. The game is played in turns. It lasts for a known number of turns. In each turn, the robot can:
- do nothing
- move to an adjacent tile (unless it is blocked)
- plant a bomb under itself
- block a tile under itself

Every time the robot dies, it is respawned in a random location and the player controlling it gets a point. The player with the least number of points is the winner.

The game is played cyclically - after a sufficient number of players have accumulated, a new game begins on the same server.

### GUI player control

- `W`, `up arrow` - move up;
- `S`, `down arrow` - move down;
- `A`, `left arrow` - move left;
- `D`, `right arrow` - move right;
- `space`, `J`, `Z` - plant a bomb;
- `K`, `X` - block a tile.

### Game architecture

Game consists of 3 parts:
- server,
- client,
- GUI.

The client communicates with the GUI and the server concurrently. It receives player input from the GUI and sends corresponding messages to the server. Additionally, it receives information about the game from the server and sends it to the GUI so it can display it to the player.

The task was to implement the server and the client. The GUI was given by the task's author and it is not in this repository.

### Usage

#### Getting GUI

Firstly, you need the GUI. To compile it, you must have Rust compiler installed. If you don't have it installed already, execute
```
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
```
and follow the onscreen instructions. Some [libraries](https://github.com/bevyengine/bevy/blob/main/docs/linux_dependencies.md) might be required.

Then you need to clone a repository containing the GUI:
```
git clone https://github.com/agluszak/mimuw-sik-2022-public.git
```

#### Build

Clone this repository (not in the directory of repository with GUI), go to its directory and run:
```
mkdir build && cd build && cmake .. && make
```

#### Run

To play a game with $N$ players you need to open $2N + 1$ terminals. One for the server and two for each player (one for the GUI and one for the client). Doing it on a single computer is quite incovenient but nothing can be done about.

##### Server

Let's try a single player game. Firstly, run the server. Server gets a few command line parameters. To get information about them, go to the `build` directory and run:
```
./robots-server -h
```
Most of them are self-explanatory and even if they aren't, experiment with the game to find out their meaning. I'd like to only point out that `<turn_duration>` is in milliseconds and `<seed>` is used just for randomness. Let's finally run the server in the first terminal:
```
./robots-server -b 4 -c 1 -d 1000 -e 3 -k 5 -l 20 -n single_player_server -p 2001 -s 41 -x 10 -y 10
```
The server is running on port 2001.

#### GUI

To use the GUI open the second terminal, go to repository containing it and run:
```
cargo run --bin gui -- --client-address localhost:2003 --port 2002

```
It runs on port 2002 and expects the client to run on port 2003. The first launch of the GUI can take some time as it is compiling.

**Warning:** GUI should be launched before the client.

#### Client

Firstly, check command line parameters that the client takes. Open the third terminal, go to the `build` directory and run:

```
./robots-client -h
```
Meaning of these parameters is obvious. Let's run the client
```
./robots-client -d localhost:2002 -n player1 -p 2003 -s localhost:2001
```
It runs on port 2003, as the the GUI expects.

If everything goes well, you should see `player1` in the lobby of the game (on the GUI). Click `space` to start the game.

#### More players

You can try to run the game with two players but it is really inconvenient on a single computer. It's much better to run clients on different machines and it is how this game should be played, as it is the network game. Note that you can use IP addresses as well as DNS domains in the command line parameters
