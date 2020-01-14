## Zaber device data plotting tool
This program is a plotting/graphing tool designed to easily interact with Zaber devices. 
It is primarily a tool to run the scope function on a device with any settings, then visually represent the data in an easy to view way.
The data can be exported as well as imported for later viewing.

## How to use
#### Running the scope
Once connected to a device, the scope command can be run by enabling one or more of the channels in the scope channels tab. To send some commands right before
scoping starts, click the "edit script button" to input the commands. After clicking the start button, the scope function will be ran and the data plotted. 
This data can be exported for later viewing.
#### Live channels
The live channel tab works in a similar way to the scope channel tab, but rather than scoping, settings are simply polled as fast as possible and graphed in real time.
The speed and data rate of this mode is limited by the number of channels being used as well as the size of the data being read. If axis scope is enabled, get commands
will be sent with the axis included which enables polling of settings on individual axes.

Warning: When live polling has started, anything typed into the setting name textbox will be sent! Only enable channels with correct setting names or the live polling
will be stopped upon the first rejected command response. Similarly, only enable axis scope settings if the setting is actually axis scope.

## Loading exported data
Without connecting to a device, the plotter can still be used to view previously saved data. this can be done by selecting "load" from the 
file menu. Any loaded graph data will appear as a channel in the live channels tab.

## How to navigate a graph
- Use the scroll wheel to zoom in and out of the graph
- Right click and drag to pan around a zoomed graph
- Hold the control key to adjust the timescale of the graph with the scroll wheel
- hold the shift key for easy interpolation of data points
- To view individual readings. zoom in closely until red circles appear, then hover over them with the mouse. 

## Using multiple graphs and dealing with different data scales
If you are viewing two channels with different data units, you can split them into multiple graphs. Do this by assigning different values to the "Graph ID" field of each channel.
Data on different channels will be shown on different graphs at full scale. As many graphs will be drawn as there are different graph IDs being used. When multiple graphs are shown, hover the mouse
in the area between them to resize them vertically as desired.

## How to install on Linux
#### Installing dependencies
**GTK**
This is required for native file dialogs.

Install with the terminal command:  `sudo apt install libgtk-3-dev`

**X11**
Window management and OpnGL context creation is handled by GLFW. For linux however, GLFW must be built with 
a rendering framework as detailed here: https://www.glfw.org/docs/latest/compile_guide.html#compile_deps
Either x11, Wayland, or OSMesa can be used, though development has only been tested with X11.

Install X11 packages with the terminal command: `sudo apt install xorg-dev`

GLFW also needs the glu library to compile from source: `sudo apt install libglu1-mesa-dev`

#### Building
- Clone this repository somewhere on to your machine.
- In a terminal window, navigate to the repository folder.
- Create a folder called "build" or run `mkdir build`
- Change directory to the new folder `cd build`
- If you don't already have cmake installed, do so now with `sudo apt install cmake`
- Start cmake with `cmake ..`
- Begin the building process by calling `make`

#### Running the program
Once built, a shared object file called "zaberplotter" will be created.
This file can be executed in a number of ways, the most simple of which is to run the file through the terminal. If you are already in the build directory,
simply run the command `./zaberplotter` to begin execution. This can be made into a shell script for ease of access.


## List of third party libraries used for this project
- https://github.com/mlabbe/nativefiledialog (native file dialogs) (Zlib)
- https://github.com/brofield/simpleini (INI file tools) (MIT)
- https://github.com/ocornut/imgui (GUI system) (MIT)
- https://github.com/glfw/glfw (OpenGL context creation) (Zlib)
- https://github.com/skaslev/gl3w (OpenGL profile loading) (unlicensed)