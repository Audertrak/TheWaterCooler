## Enjenir (in development title)

A turn-based, puzzle/strategy card based simulator for Engineering principles.

### Philosophy

From a _design_ perspective, the core concept is using the smallest set of primitives that lead to complex, emergent and engaging scenarios (the most “bang for buck” / Pareto Principle / 80-20 rule). Taking inspiration from computers/digital design, nearly every calculation can be performed by strategically arranging switches, and very complex problems can be simulated/abstracted as boolean logic equations. With this “most from least” approach, the powerful primitives of boolean logic, and Engineering as the inspiration: “under the hood” the user is engineering boolean logic structures to create a Turing machine able to meet some specification.

### Terminology

The application should use its own neutral/agnostic terms that are distinct but recognizable.This enables us to not do be “domain specific” to a particular discipline of Engineering, and will help make the game more approachable/digestible to users without an Engineering background.

The application should explicitly avoid any reference to “game”, “play”, etc. Instead, the UI/UX should present itself to the user as enterprise software, simulation software, or _maybe_ educational software.

#### Core Concepts

Build and optimize **systems** to perform some **transformations** through strategic combinations of **components, nodes,** and **modules** (collectively called **elements**) via **signals** over **links** to achieve some level of **capability**.

*   Components: simplest element type, either produces some output _or_ accepts some signal input
    *   analogues: battery, generator, light bulb, speaker
*   Link: allows the connection between two elements
    *   analogues: wire, pipe
*   Signal: sent over links between elements
    *   analogues: electricity, data, pressure
*   Node: a discrete element with both some input and some output
    *   analogues: valve, traffic light
*   Module: two or more linked nodes
    *   analogues: half adder, assembly line, intersection
*   Transformation: inversion, addition, subtraction, multiplication, or division of signal
*   System: all connected elements placed on the canvas
    *   analogues: city, integrated circuit, machine shop, vehicle
*   Capability: the maximum enumerated list of _unique_ global output states of the system
    *   analogues: finite state machine, schematic, specification, capacity
*   Efficiency: ratio of possible unique global output states divided by the number of output only components
    *   analogues: throughput, instructions per clock, torque

Components represent **points** on the **edge** of a system. A component that produces some output is considered an **origin point**. Origin points cannot perform any transformation, but no transformation can occur without at least one origin. A component that accepts some input is considered a **terminal point**. Terminal points perform a single transformation, but cannot do so unless connected (directly or indirectly) to an origin. The collection of all origin points represents the **global origin** of the engine, and the collection of all terminal components represents the **global terminal** of the engine. The complete enumeration of all discrete possible **states** at the global terminal represents the **capability** of the system, or in other words, the range of transformations that can be achieved by the system. The ratio of possible states at the global terminal to the number of points on the global origin represents a system's **efficiency**.

Links are **polar**; it is not possible to connect an input←→input or output←→output.

Conceptually, a module can be considered a node. This means it would be a possible to have a module, composed of modules, which when considered at a macro level is also a node. 

### UI

The goal is to invoke a memorable and thematic look and feel deeply rooted in engineering concepts, drawing inspiration from iconic engineering software interfaces, technical documentation styles (blueprints, schematics, diagrams), Bauhaus, and retro-futuristic mid-century modern aesthetics. UI should lean heavily into the use of typography, and aesthetic ratios/proportions.

Main application UI is a “virtual whiteboard”, a pseudo “infinite canvas”. The imagery is meant to evoke a corporate office whiteboard, drafting table, or digital design environment upon which the application elements (cards, UI panels, indicators) are placed, arranged, and interacted with. UI elements (like hand, deck, discard piles, resource indicators) appear as distinct panels or areas anchored to the screen, akin to tool palettes, docked windows in engineering software, or diagrams drawn on a whiteboard. Playing cards, triggering effects, and managing resources should visually feel like drawing, annotating, connecting, placing, or manipulating components on this technical surface.

All game visual assets, including card "illustrations," UI icons, and visual effects, should adhere to a technical, diagrammatic style.

*   Orthographic or Isometric projections of components.
*   Circuit schematic symbols.
*   Process flow diagram shapes and connectors.
*   Technical graphs and charts.
*   Mechanical assembly diagrams.
*   Focus on conveying information and function through clear lines and shapes rather than photorealistic depiction.
*   Use standard technical drawing conventions where appropriate (e.g., dashed lines for hidden components, specific arrow types for flow/direction).

To reinforce the technical, typewriter-like feel, the primary typeface will be monospaced. Currently the plan is to use Courier family fonts.

Resources**:**

*   Rectangular shape in the proportion of standard copy paper (8.5 x 11) with clean, defined borders.
*   Layout within the card should be clear: Title area (monospaced font, potentially bold), "Illustration" area (technical diagram/symbol), Effect Text area (monospaced font), Cost/Resource icons (simple, geometric shapes or technical symbols).
*   Card borders or backgrounds can subtly change based on card type (e.g., component, action, upgrade) but should remain within the overall technical aesthetic of the chosen theme.

### Gameplay

Users are allocated a randomized set of resources (cards) from a “deck”. and their available resources determines what they are able to accomplish each turn.

The “deck” consists of a mix of element and action/effects resources. Elements are persistent once placed. Action and effect resources are exhausted when used and discarded. If the deck is exhausted, all discarded resources are randomized and become the new “deck”.

A set of conditions (effectively a finite state machine), referred to as a “Scenario”, is defined. Possible conditions could include minimum or maximum numbers of input components, output components, nodes, unique states, state combinations, etc. The user is tasked to construct a system capable of satisfying the scenario.

The calculated capability and efficiency of the users system is displayed on the banner, as well as their progress toward the scenario conditions.

The user can click a button on the banner which displays a more detailed representation of the system (finite state machine) they are tasked with creating.

#### Resources

Note that these are ideas for resources that could be available to user, and this list is subject to grow or shrink and the listed resources themselves may be altered.

##### Elements

###### Basic Components

*   Source:
    *   single output
    *   when played, always outputs a signal
*   Button:
    *   single output
    *   only outputs a signal when clicked by the user
    *   can be clicked at any time
*   Switch:
    *   single output
    *   when first played, outputs no signal
    *   when clicked by the user, output is ‘toggled’ to the next state
    *   can be clicked at any time
*   Sensor:
    *   single input
    *   triggered by a signal
    *   adds to capability

###### Advanced Components

*   Sequencer:
    *   single input
    *   no default behavior when first played.
    *   when clicked, a dialogue opens allowing the user to modify the stored sequence (4 signals) to either output ON or OFF at that step of the sequence
    *   plays back the stored sequence whenever it receives a signal
    *   significantly adds to capability

###### Basic Nodes

*   NOT:
    *   single input, single output
    *   outputs the opposite of the input signal
*   AND:
    *   double input, single output
    *   outputs a signal if and only if it receives two simultaneous signals
*   OR:
    *   double input, single output
    *   outputs a signal if it receives at least one signal
*   Bus:
    *   single input, quad output
    *   repeats inputs signal across all outputs

###### Advanced Nodes

*   Flip Flop:
    *   single input, single output
    *   similar to a switch, but cannot be toggled by the user. Instead, when a flip-flop receives a signal, its output state is toggled.
*   Mux:
    *   5 inputs, single output.
    *   One input acts as the “select”.
    *   Whenever the select receives signal, it increments which signal input is connected to the output by 1
*   Tape:
    *   similar to a sequencer, but acts as a single input, single output node.
    *   When played, outputs no signal.
    *   When clicked, a dialogue opens allowing the user to modify the stored sequence (4 signals) to either output ON or OFF at that step of the sequence.
    *   Plays back the stored sequence whenever it receives a signal as an output

##### Action/Effect

*   Requisition: draw 3 cards
*   Recycle: discard any number of remaining resources from hand; draw that many resources
*   Re-Org: discard hand; draw to full hand
*   Job Fair: 'card pack'; optionally pick one of 3 randomized resources to be permanently added to deck, Job Fair is discarded
*   Continuous Improvement: select an element, add an input or output
*   End of Life: select a card from hand; selected card is permanently removed and EoL is discarded
*   Parts Bin: select an element currently in play; user may place a copy of selected element and Parts bin is discarded
*   Blueprint: When selected, the user selects 2 adjacent elements currently in play (a module), and the player can play a copy of the selected module as if it were a single node; blueprint resets and is discarded
*   Schematic: similar to blueprint. When selected, user selects any 1 or 2 adjacent elements, and the schematic copies the configuration, after which the schematic is discarded. When next drawn, the user can either choose to play the current schematic as if it were a single node, or choose two new objects to copy

## Development

*   C language (c11 standard, unless c17 or c23 would offer specific benefits for this application)
    *   Our .h header files should use doxygen comments and contain documentation/explanations/instructions for that header file and its corresponding .c implementation file
    *   Our .c implementation files should have **no** comments, at all, whatsoever
        *   in config.h there is a ‘debug’ macro that should be used instead of comments
            *   `TODO` used to document partial implementations and/or future implementations
            *   `STUB` used to suppress compiler warnings as the ‘body’ of a stubbed feature/function
*   Raylib and Raygui for client application UI (and/or any other of the modules offered by Raysan)
*   I prefer to use ‘stb style’ single header only libraries when possible
*   I prefer single file "unity builds", or as close as possible (basically I want to minimize linking whenever possible and instead just directly include “source files”
*   I am developing on Windows, but I would like be stub out if/when required for Linux
    *   I can build on Linux so I don't need to cross compile but I will be compiling with zig on Windows so building to different targets is an option
    *   This 'cross platformness' means ideally using cross platform libraries, or having to handle conditional compilation
        *   I would like to avoid a bunch of conditional compilation handling if at all possible
*   Modular architecture; pseudo 'client-server', primarily based around the Raylib API, extending where necessary
    *   The "server" module handles the core gameplay logic and keeps track of the current application state
    *   The "server" module sends a buffer/message/string over a socket/connection that is interpreted by the "client" module and used to ‘hydrate’ the UI canvas via the Raylib API
    *   Goal is for the ‘server’ module (core engine/logic) to eventually be compiled standalone and allow development of different clients (lua/love2D, go, browser-based, python, etc) by using Raylib bindings and/or implementation the game API for the target
*   “Native” clients should be fully self contained, statically linked executables
*   Make for build tool
    *   Build targets
        *   Native
            *   Zig compilation
                *   Windows
                *   Linux
                *   Android app
                *   standalone http server for web build
        *   Emscripten compilation
            *   web

#### Structure (so far)

*   `src/main.c`: Entry point, orchestrates the main game loop
*   `src/server.h`/`src/server.c`: Manages core game state and logic (components, cards, deck, player interactions). Designed to be potentially separable for different client implementations
*   `src/client.h`/`src/client.c`: Handles all Raylib rendering, UI, input processing, and visual representation of the game state. Includes RayGui for UI elements
*   `src/config.h`: Centralized definitions for screen dimensions, colors, font paths, UI layout constants, and debug macros (TODO/STUB)
*   `Makefile`: supports building native executable, web target, and ‘compile\_commands.json’ for lsp/compiler assistance
*   `/lib`: directory with external dependencies (e.g. Raylib source code/library)
*   `/assets`: currently has folders for fonts and icons

#### Features Implemented (so far)

*   Basic window creation and management
*   Game state loop with three ‘scenes’: Logo, Title, Gameplay
*   Custom font loading and basic colorscheme
*   3 main Gameplay UI elements… header, play area ('canvas'), deck/hand area
    *   Header to display the current scenario/gameplay/game state information
    *   Main canvas can be magnified and dragged for enhanced visibility
    *   Cards are displayed in the deck/hand area with information
*   Cards
    *   Deck is initialized with a number of cards on startup
    *   Cards can be selected then played to the canvas
        *   are discarded after play
    *   Switch, AND, and OR card types
        *   ‘switch’ objects can be turned on/off by clicking them
        *   AND and OR objects properly update based on inputs and output
        *   All objects represent their current state through changing colors
    *   card draw by pressing ‘D’
*   Wiring mode: pressing ‘W’ allows user to draw ‘wires’ between two components

* * *

> [!CAUTION] 
> **IMPORTANT INSTRUCTIONS: DO NOT IGNORE**
> - **Do not provide explanations unless I specifically ask a clarification question.** The goal here is **productivity**, not learning.
> - **Do not, under any circumstance, for any reason, at any time, provide broken code**
>   * When providing code to be inserted or modified, make sure to include the full code implementation for the referenced scope, with instructions or a reference to the previous and following code (if applicable).
>   * This does not mean you have to provide a full updated version of the file every response. What it does mean, is that with a snippet, block, etc, you should never replace code with comment like `// same as before`
>   * Any code block or section you provide should be able run assuming it is copied and pasted into the correct location in the file
> - **Do not, under any circumstance, for any reason, at any time, include comments when providing code for .c files**
>   * In place of comments, use the TODO and/or STUB macros from config.h
>	    + TODO: basically prints a message telling that this feature needs to be implemented and how to do so (e.g. fix text alignment, replace with sprite/texture/ etc)
>		+ STUB: used to suppress compiler warnings for non-essential or low priority features (e.g. android client, cross compilation, etc), prints a message as a reminder
> - **Use doxygen comments and comment blocks with documentation/explanations/instructions in .h files that explain how to use, update, and modify both the header file and its corresponding .c implementation file**
