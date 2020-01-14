#include <glm/glm.hpp> //math


#include <iomanip>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

#include <imgui/imgui.h>

#include "configuration.h"
#include "gui_utils.h"
#include "imgui_internal.h"

#include <nfd.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
using namespace glm;
using namespace ImGui;

static volatile FileDialogEvent currentFileDialogStatus = FileDialogEvent::NOT_OPEN;

thread fileDialogThread;
void fileDialogThreadFunc(bool openFile, char * filePath){
	nfdresult_t result;
	nfdchar_t *outPath = NULL;
	if(openFile)
		result = NFD_OpenDialog( NULL, NULL, &outPath );
	else
		result = NFD_SaveDialog( NULL, NULL, &outPath );
        
    if ( result == NFD_OKAY ) {
        puts("Success!");
        puts(outPath);
		strcpy(filePath, outPath);
		currentFileDialogStatus = FileDialogEvent::OKAY;
		free(outPath);
    }
    else if ( result == NFD_CANCEL ) {
        puts("User pressed cancel.");
		currentFileDialogStatus = FileDialogEvent::CANCEL;
    }
	else{
		printf("Error: %s\n", NFD_GetError());
		currentFileDialogStatus = FileDialogEvent::ERROR;
	}
}

void saveFileDialog(char * filePath){
	if(currentFileDialogStatus == FileDialogEvent::NOT_OPEN){
		if(fileDialogThread.joinable())
			fileDialogThread.detach();
		currentFileDialogStatus = FileDialogEvent::OPEN;
		fileDialogThread = thread(fileDialogThreadFunc, false, filePath);
	}
}
void openFileDialog(char * filePath){
	if (currentFileDialogStatus == FileDialogEvent::NOT_OPEN){
		if(fileDialogThread.joinable())
			fileDialogThread.detach();
		currentFileDialogStatus = FileDialogEvent::OPEN;
		fileDialogThread = thread(fileDialogThreadFunc, true, filePath);
	}
}

FileDialogEvent pollFileDialogEvent(){
	return currentFileDialogStatus;
}

void clearFileDialogStatus(){
	if(currentFileDialogStatus == FileDialogEvent::OPEN)
		fileDialogThread.~thread();
	currentFileDialogStatus = FileDialogEvent::NOT_OPEN;
}

// A button which can be disabled
bool toggleButton(const char * label, bool enabled) {
	const ImColor grey(0.5f, 0.5f, 0.5f, 1.0f);

	if (enabled)
		return Button(label);

	// Render a grey button and disregard click state.
	PushStyleColor(ImGuiCol_Button, (ImVec4)grey);
	PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)grey);
	PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)grey);
	Button(label);
	PopStyleColor(3);
	return false;
}

bool Splitter(unsigned int id, bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
	char idBuff[32];
	sprintf(idBuff, "##Splitter%d", id);
	ImGuiID actualId = window->GetID(idBuff); //GetID("##Splitter");
    ImRect bb;
    bb.Min = vec2(window->DC.CursorPos) + vec2((split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1)));
    bb.Max = vec2(bb.Min) + vec2(CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f));
    return SplitterBehavior(bb, actualId, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

bool horizontalNavbar(float * val, float min, float max) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	vec2 canPos = ImGui::GetCursorScreenPos();

	vec2 canvasSize = ImGui::GetContentRegionAvail();
	canvasSize.y = 30;

	float width = 50;

	drawList->AddRectFilled(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), IM_COL32(25, 25, 30, 255));


	ImGui::InvisibleButton("navBar", canvasSize);
	vec2 mpos = ImVec2(ImGui::GetIO().MousePos.x - canPos.x, ImGui::GetIO().MousePos.y - canPos.y);

	vec2 cornerA = (vec2(canPos.x + (*val - width / 2.0f), canPos.y));
	vec2 cornerB = vec2(canPos.x + (*val + width / 2.0f), canPos.y + canvasSize.y);

	cout << mpos.x << " " << mpos.y << endl;
	cout << cornerA.x << " " << cornerA.y << endl;
	cout << cornerB.x << " " << cornerB.y << endl;


	bool hover =  ImGui::IsMouseHoveringRect(cornerA, cornerB);
	auto col = hover ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255);
	if (hover)
	{
		int tetset = 0;
	}

	drawList->AddRectFilled(cornerA, cornerB, col);
	return true;
}

bool dialogOpen = false;
char popUpMessage[2048];

void createPopupDialog(char * message){
	if(!dialogOpen){
		dialogOpen = true;
		OpenPopup("Message");
		strcpy(popUpMessage, message);
	}
}

void showPopupDialog(){

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoResize;

	if(dialogOpen)
		SetNextWindowContentWidth(280);

	if(BeginPopupModal("Message", &dialogOpen, window_flags)){
		Text(popUpMessage);
		if(Button("Okay"))
			dialogOpen = false;
		EndPopup();
	}
}