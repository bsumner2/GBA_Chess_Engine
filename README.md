# GBA_Chess_Engine

## Brief Summary
A Chess engine written entirely by me (yes, even the static library in root/lib!
You can find the source code for it at this [repo](https://github.com/bsumner2/GBA_Dev)!)

## Game So Far

After months of incremental bursts of progress, I finally have completed a bulk
of the game:
- Title screen two choose between gameplay modes
- Gameplay
    - Local two Payer (Person x Person)
    - Singleplayer
        - Person x CPU
        - CPU x person
- (White|Black) Victory Screen & Stalemate Screen
- Loopability of gameplay
- CPU Engine Features (requires recompilation with predefs, see
  specific Makefile predef commands for each optional feature listed farther below):
    - Engine Analysis Features
        - Engine Movement Visualizer
        - Decision Tree Traversal using GBA Keypad to step through DFS traversal
    Engine Behaviour Features
        - Change maximum depth of Engine's decision tree

### Gameplay Footage

Below is a GIF of the game compiled with The CPU Engine movement visualizer
setting.

![alt text](/demo/GBA_Chess_Engine_Screenrecording1.gif "Gameplay of title screen + gameplay w/ engine decision tree visualizer")

Below is a GIF of the game running with a custom CPU maximum depth, 
of MAX_DEPTH=1, and Engine analysis features turned off. 
Max depth is the fastest, but it isn't the sharpest tool in the shed. Engine
only starts getting smart at MAX_DEPTH=3


![alt text](/demo/GBA_Chess_Engine_Screenrecording0.gif "Gameplay of title screen + gameplay w/ CPU Max Depth=1 and Analysis turned off")

### Build commands for custom predefs:

#### Engine Movement Visualizer Predef:

Predefine this feature with preprocessor macro *_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_*
e.g.:

```shell
$ make clean build MACROS="-D_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_"
```

#### CPU Opponent Engine Decision Tree Traversal Via Keypad

Predefine this feature with preprocessor macro *_DEBUG_BUILD_*
e.g.:

```shell
$ make clean build MACROS="-D_DEBUG_BUILD_"
```

#### Custom Max Depth

Predefine this feature with preprocessor macro *MAX_DEPTH=<max depth number>*
e.g.:

```shell
$ make clean build MACROS="-DMAX_DEPTH=4"
```

