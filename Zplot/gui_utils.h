#pragma once

#include <glm/glm.hpp> //math


#include <iomanip>
#include <string>
#include <vector>

#include <imgui/imgui.h>

#include "configuration.h"

// Shows a native save file dialog on a separate thread.
void saveFileDialog(char * filePath);

// Shows a native open file dialog on a separate thread.
void openFileDialog(char * filePath); 

enum class FileDialogEvent{
    OPEN,
    NOT_OPEN,
    OKAY,
    CANCEL,
    ERROR
};

// Gets the current status of any currently open file dialogs.
FileDialogEvent pollFileDialogEvent();

// Reset the dialog status and allow a new dialog to be opened. Kills the dialog thread if one is already open.
void clearFileDialogStatus();

// A button which can be disabled.
bool toggleButton(const char * label, bool enabled);

// Vertical resizable region.
bool Splitter(unsigned int id, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

//bool horizontalNavbar(float * val, float min, float max);
bool horizontalNavbar(float * val, float min, float max);

// Creates a very simple popup modal dialog. Only one can be shown at a time!
void createPopupDialog(char * message);

// Displays the modal dialog
void showPopupDialog();