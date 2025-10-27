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
        - Engine Decision Tree Visualizer
        - Decision Tree Traversal using GBA Keypad to step through DFS traversal
        - Engine Move Options Visualizer: Similar to Decision Tree Visualizer,
          with only diff being that it only highlights the move choices for
          AI's current turn, but not any of the follow up move analysis.
    - Engine Behaviour Features
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

#### Engine Decision Visualizer Predef:

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

#### Engine Move Options Visualizer

Predefine this feature with preprocessor macro *_AI_VISUALIZE_MOVE_CANDIDATES_*

```shell
$ make clean build MACROS="-D_AI_VISUALIZE_MOVE_CANDIDATES_"
```

#### Mix and match features with build var predef for MACROS:

- *MAX_DEPTH* macro can be mixed and match with any other macros
- *_AI_VISUALIZE_MOVE_CANDIDATES_* and *_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_* 
  are mutually exclusive, meaning you can't enable both. Only one or the other.
- *_DEBUG_BUILD_* macro automatically enables 
  *_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_*, and cannot choose *_DEBUG_BUILD_*
  with *_AI_VISUALIZE_MOVE_CANDIDATES_*, which is incompatible with 
  *_AI_VISUALIZE_MOVE_SEARCH_TRAVERSAL_*

So basically, MAX_DEPTH is compatible with any other feature, but the rest 
should be treated as mutually exclusive. In the future, I'm gonna add a 
feature to let these all be available in-game, and features can be configured
during runtime.

##### Examples:

Enable Stepwise Decision Tree Traversal and set custom max depth of 4

```shell
$ make clean build MACROS="-DMAX_DEPTH=4 -D_DEBUG_BUILD_"
```
