#define _CRT_SECURE_NO_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING

#include <stdio.h>
#include <math.h>
#include <assert.h> 

#include <gl3w/gl3w.h> //opengl profile loader
//#include <GL/GLU.h> //need for opengl
#include <GLFW/glfw3.h> //opengl context

#include <glm/glm.hpp> //math
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <mutex>
#include "safe_que.h"

#include <imgui/imconfig.h> //user interface
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include "imgui_internal.h"

#include "communication.h"
#include "gui_utils.h"
#include "line_graph.h"
#include "file_handling.h"

using namespace std;
using namespace glm;


#include <iostream>
#include "macros.h"

const vec4 backCol(glm::vec3(0.15f), 1); //back color of the window
const char* glsl_version = "#version 330 core";

int resWidth = 2000, resHeight = 1200; // Initial and current window size.
float windowDiv = 300; // Width of the side bar.
GLFWwindow* window;
ImFont* canvasFont;
float canvasZoomAmount = 1.0f;
bool zoomChanged = false;
bool zoomEnabled = false;
float zoomSpeed = 0.2f;
float aspect;
float globalScrollDelta = 0.0f;
bool darkmode = true;

vector<dataChannel> liveChannels;
const int numScopeChannels = 6;
dataChannel scopeChannels[numScopeChannels];

// Quite usefull for parseing the scope print output from a device. 
vector<string> stringSplit(string s, string delimiter) {
	vector<string> results;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		if (token.size() > 0)
			results.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	results.push_back(s);
	return results;
}

// imgui doesn't have much for scroll input, so it's kept track of here.
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	globalScrollDelta = (float)yoffset;
	if (!zoomEnabled)
		return;

	canvasZoomAmount += yoffset * zoomSpeed;
	canvasZoomAmount = glm::clamp(canvasZoomAmount, 1.0f, 12.0f);
	zoomChanged = true;
}

// UI forwared declared functions
void mainLoop();
void leftPanel();
void graphCanvas();
void showMainMenueBar();
void showScopeChannelTab();
void showLiveChannelTab();
void showScriptEditWindow();
void showCommandWindow();
void (*fileDialogHandler)();
void saveDataFile();
void loadDataFile();
void AddLog(const char *fmt, ...);
void onReceive(char data[256]);
void (*receiveCallback)(char[256]) = &onReceive;

// UI related data
bool scriptWindowOpen = false;
bool commandWindowOpen = false;
bool figureExportWindowOpen = false;
bool liveTabSelected = false; // Which Mode is being used
char fileDialogFilenameBuffer[2048];
char modalDialogBuffer[2048]; //General use buffer for formating popup dialog messages
dataChannel * channelSaveTarget = nullptr;
thread livePollThread;

int main()
{
	//init opengl context
	int glfwInitResult = glfwInit();
	if (glfwInitResult != GLFW_TRUE)
	{
		fprintf(stderr, "glfwInit returned false\n");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	window = glfwCreateWindow(resWidth, resHeight, "Zaber Plotter", NULL, NULL);

	if (!window)
	{
		fprintf(stderr, "Failed to open glfw window. Is opengl 3.2 supported?\n");
		return 1;
	}

	// We need our own scroll callback to handle zooming in the graph views.
	glfwSetScrollCallback(window, scroll_callback);
	glfwPollEvents();
	glfwMakeContextCurrent(window);

	int gl3wInitResult = gl3wInit();
	if (gl3wInitResult != 0)
	{
		fprintf(stderr, "gl3wInit returned error code %d\n", gl3wInitResult);
		return 1;
	}

	gl(Enable(GL_BLEND));
	gl(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	// vsync and framerate locking.
	glfwSwapInterval(1);

	// Grab io
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	// Text on an imgui drawlist requires a font object. Might as well get the built in one.
	canvasFont = io.FontDefault;
	LineGraph::labelFont = canvasFont;

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImGui::StyleColorsDark();

	while (!glfwWindowShouldClose(window)) {
		mainLoop();
		globalScrollDelta = 0.0f;
	}

	if(livePollThread.joinable())
		livePollThread.detach();

	return 0;
}


//opengl context and window managing busywork
bool firstFrame = true;
double currentTime = 0;
double currentFrameTime() {
	return glfwGetTime() - currentTime;
}

// Some common colors
#define white (ImColor)IM_COL32(255, 255, 255, 255)
#define red (ImColor)IM_COL32(255, 0, 0, 255)
#define black (ImColor)IM_COL32(0, 0, 0, 255)
#define orange (ImColor)IM_COL32(239, 136, 95, 255)
#define blue (ImColor)IM_COL32(25, 104, 232, 255)
#define yellow (ImColor)IM_COL32(255, 255, 0, 255)
#define transparent ImColor)IM_COL32(255, 255, 255, 100)
#define invisible (ImColor)IM_COL32(0, 0, 0, 0)


// General data.
char reply[256] = { 0 }; // Used to temporarily store replies from device.
int pointsInTheLastSecond = 0;
int pointsPerSecond = 0;
bool shouldAbortStreamThread = false;
volatile bool streamThreadRunning = false;
bool autoScroll = false;
int activeScopeChannels = 0;
int activeLiveChannels = 0;
const int maxGraphs = 10;
mutex dataLock;

// Text box buffers
char portName[512] = "/dev/ttyUSB0";
char liveChannelSettingBuff[256];

// When the live poll thread is running, send commands to this que instead of sendAndWait()
// so the live poll doesn't get the reply data mixed up. Mutexes were really
// slow, so this solution works much better provided the reply data isn't needed. 
SafeQueue<std::string> commanInterjections;

//note: add safety to min and max point calculations
void livePollThreadFunc() {
	using namespace std;
	using namespace chrono;
	using namespace communication;

	streamThreadRunning = true;
	pollUntilIdle();

	clock_t begin = clock();

	high_resolution_clock::time_point t1 = high_resolution_clock::now();

	while (true) {

		// User pressed stop.
		if (shouldAbortStreamThread) {
			if (isConnected()) {
				char unused[256];
				while (receive(unused));
			}
			streamThreadRunning = false;
			shouldAbortStreamThread = false;
			return;
		}

		// User has commands they want to send while live channels are running.
		while(commanInterjections.size() > 0){
			char command[256];
			strcpy(command, commanInterjections.dequeue().c_str());
			sendAndWait(command);
		}

		for (int i = 0; i < liveChannels.size(); i++)
		{
			if (!liveChannels[i].enabled)
				continue;

			float dataPoint;

			if(liveChannels[i].axisScopeEnabled){
				sprintf(liveChannelSettingBuff, "/0 %d get %s\n", liveChannels[i].axis, liveChannels[i].settingName);
			}
			else{
				sprintf(liveChannelSettingBuff, "/get %s\n", liveChannels[i].settingName);
			}
			
			// If the get command fails for any reason, stop polling.
			if(!sendAndWait(liveChannelSettingBuff, &dataPoint)){
				streamThreadRunning = false;
				AddLog("Device error while reading setting.");
				cout << "Device error while reading setting" <<endl;
				return;
			}
			pointsInTheLastSecond++;

			high_resolution_clock::time_point t2 = high_resolution_clock::now();
			duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
			double timePoint = time_span.count();

			// Don't mess with channels while the UI thread is updating.
			dataLock.lock();
			if (liveChannels[i].dataPoints.size() == 0) {
				liveChannels[i].minPoint = dataPoint;
				liveChannels[i].maxPoint = dataPoint;
			}
			else
			{
				liveChannels[i].minPoint = glm::min(dataPoint, liveChannels[i].minPoint);
				liveChannels[i].maxPoint = glm::max(dataPoint, liveChannels[i].maxPoint);
			}
			liveChannels[i].dataPoints.push_back(dataPoint);
			liveChannels[i].timePoints.push_back(timePoint);
			liveChannels[i].pointCount = liveChannels[i].dataPoints.size();
			dataLock.unlock();
		}

		clock_t end = clock();
		double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
		if (elapsed_secs > 1) {
			begin = clock();
			pointsPerSecond = pointsInTheLastSecond;
			pointsInTheLastSecond = 0;
		}
	}
	streamThreadRunning = false;
}

void abortStreamThread() {
	shouldAbortStreamThread = true;
}

// Finds if something exists in a vector.
template<typename T>
bool contains(vector<T> v, T value) {
	for (int i = 0; i < v.size(); i++)
		if (v[i] == value)
			return true;
	return false;
}

// This runs once per frame (up to 60 times per second)
void mainLoop() {
	glfwMakeContextCurrent(window);
	currentTime = glfwGetTime();

	// Update window related things and poll user input
	glfwPollEvents();

	glfwGetWindowSize(window, &resWidth, &resHeight);
	vec2 resolutionV2f = vec2((float)resWidth, (float)resHeight);
	aspect = resolutionV2f.y / resolutionV2f.x;

	gl(Viewport(0, 0, resWidth, resHeight));
	gl(ClearColor(backCol.r, backCol.g, backCol.b, 1.0f));
	gl(Clear(GL_COLOR_BUFFER_BIT));

	// Check any file dialogs that are currently open.
	FileDialogEvent dialogStatus = pollFileDialogEvent();
	switch (dialogStatus)
	{
	case FileDialogEvent::NOT_OPEN :
		// Nothing to do here.
		break;
	case FileDialogEvent::OPEN :
		// Still being used. Just leave it open. clearFileDialogStatus() could be called here to force close it though.
		break;
	case FileDialogEvent::CANCEL :
		// User changed their mind. Allow new dialog to be opened.
		clearFileDialogStatus();
		break;
	case FileDialogEvent::OKAY :
		// Handle the saveing/loading operation. Also allow new dialog to be opened.
		(*fileDialogHandler)();
		clearFileDialogStatus();
	case FileDialogEvent::ERROR :
		// Should probably do something else here.
		clearFileDialogStatus();
		break;
	default:
		break;
	}

	// Start drawing the GUI
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	using namespace ImGui;

	showMainMenueBar();
	leftPanel();

	if(scriptWindowOpen){
		showScriptEditWindow();
	}
	if(commandWindowOpen){
		showCommandWindow();
	}

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoNav;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoScrollbar;
	window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
	window_flags |= ImGuiWindowFlags_NoDocking;

	Begin("empty", NULL, window_flags);

	SetWindowSize(vec2(resWidth - windowDiv, resHeight));
	SetWindowPos(vec2(windowDiv, 0));

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0;

	showPopupDialog();
	graphCanvas();	

	End();

	// Rendering
	ImGui::Render();
	int display_w, display_h;
	glfwMakeContextCurrent(window);
	glfwGetFramebufferSize(window, &display_w, &display_h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	gl(UseProgram(0));
	gl(BindVertexArray(0));

	glfwSwapBuffers(window);
	firstFrame = false;
	zoomChanged = false;
}

const float minGraphHeight = 200;
LineGraph liveGraphs[maxGraphs];
float regionSizes[maxGraphs];

void graphCanvas() {
	using namespace ImGui;
	std::lock_guard<std::mutex> lock(dataLock);
	// Find how many different graphs are needed and what the unique IDs being used are.
	int channelCount = liveTabSelected ? liveChannels.size() : numScopeChannels;
	vector<int> graphIDs;
	for (int i = 0; i < channelCount; i++) {
		dataChannel channel = liveTabSelected ? liveChannels[i] : scopeChannels[i];
		if (!channel.visible)
			continue;

		if (!contains<int>(graphIDs, channel.graphID))
			graphIDs.push_back(channel.graphID);
	}
	int graphCount = graphIDs.size();
	float canvasHights = ImGui::GetContentRegionAvail().y / graphCount;
	float availHeight = GetContentRegionAvail().y;


	// Handle sizes of the graphs so that they all fit in the screen and don't waste any space.
	if (graphCount == 1) {
		// Just use the whole screen for one graph.
		regionSizes[0] = availHeight;
	}
	else {
		// Figure how bad the combined heigh under or over shoots the window size.
		float regionCombinedHeight = 0;
		for (int i = 0; i < graphCount; i++)
			// Treat any new graphs with zero height as if they were the minimum height 
			regionCombinedHeight += glm::max(minGraphHeight, regionSizes[i]);

		// Resize regions from bottom up until they all fit
		int adjustmentIndex = graphCount -1;
		while(regionCombinedHeight > availHeight && adjustmentIndex >= 0){
			float overscan = regionCombinedHeight - availHeight;
			regionSizes[adjustmentIndex] = glm::max(minGraphHeight, regionSizes[adjustmentIndex] - overscan);

			regionCombinedHeight = 0;
			for (int i = 0; i < graphCount; i++)
				regionCombinedHeight += regionSizes[i];
			adjustmentIndex--;
		}

		//If there is extra space (like when the window size is increased) then make the bottom graph use the reset of it.
		if (regionCombinedHeight < availHeight) {
			regionSizes[graphCount - 1] = availHeight - (regionCombinedHeight - regionSizes[graphCount - 1]);
		}

		// Que up the first splitter before the regions are created.
		Splitter(1, false, 8.0f, regionSizes + 0, regionSizes + 1, minGraphHeight, minGraphHeight);
	}

	// Render the graphs.
	for (int graphI = 0; graphI < graphCount; graphI++)
	{
		int graphID = graphIDs[graphI];
		vector<dataChannel> channelGroup;
		// Figure out what channels are being displayed one this graph.
		for (int channelI = 0; channelI < channelCount; channelI++)
		{
			dataChannel channel = liveTabSelected ? liveChannels[channelI] : scopeChannels[channelI];
			if (!channel.visible)
				continue;

			if (channel.graphID == graphID)
				channelGroup.push_back(channel);
		}
		if (channelGroup.size() > 0) {
			
			// The splitter function must be called before the two regions it's splitting are created.
			if (graphCount - graphI > 1 && graphI > 0)
				Splitter(graphI + 1, false, 8.0f, regionSizes + graphI, regionSizes + graphI + 1, minGraphHeight, minGraphHeight);

			// Every graph needs a unique ID.
			char idBuff[8];
			sprintf(idBuff, "r%d", graphI);
			
			// Render the graph in the subregion.
			BeginChild(idBuff, ImVec2(ImGui::GetContentRegionAvail().x, regionSizes[graphI]));
			liveGraphs[graphI].render(channelGroup, globalScrollDelta, canvasHights, darkmode, autoScroll && liveTabSelected);
			EndChild();
		}
		// Pretty sure I've covered every scenario here but you never know...
		else{
			printf("A branch which shouldn't be possible was reached on line %d in file %s\n", __LINE__, __FILE__);
		}
	}
}

bool colorPickerHeaders[numScopeChannels];
float timeBase = 0.1f;
float scopeDelay = 0.0f;

void leftPanel() {
	using namespace ImGui;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoResize;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoNav;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	window_flags |= ImGuiWindowFlags_NoDocking;

	Begin("no title", NULL, window_flags);

	SetWindowSize(vec2(windowDiv, resHeight));
	SetWindowPos(vec2(0));

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = vec2(25);
	style.WindowRounding = 0;

	PushItemWidth(100);
	InputText("", portName, IM_ARRAYSIZE(portName));
	PopItemWidth();
	SameLine();

	if (toggleButton("Connect", !communication::isConnected())) {
		if (!communication::connect(portName) || !communication::sendAndWait("/")){
			createPopupDialog("Could not connect to the specified device.");
			cout << "Could not connect to device.\n" << endl;
			AddLog("Could not connext to the device.\n");
			communication::disconnect();
			AddLog("Connected..."); 
		}	
	}

	SameLine();

	if (toggleButton("Disconnect", communication::isConnected())) {
		// Stop live read thread if it still running
		if (!shouldAbortStreamThread)
			abortStreamThread();
		communication::disconnect();
	}
	Checkbox("Show command window", &commandWindowOpen);
	if(Button("Reset timescale")){
		LineGraph::resetHorizontalScale();
	}
	Separator();

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("Channel Tabs", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Scope Channels"))
		{
			liveTabSelected = false;
			Separator();
			showScopeChannelTab();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Live Channels"))
		{
			liveTabSelected = true;
			Separator();
			showLiveChannelTab();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	End();
}

void showLiveChannelTab() {
	using namespace ImGui;
	// Deals with the same data from the live poll thread.
	std::lock_guard<std::mutex> lock(dataLock);

	bool anyChannelsEnabled = false;
	for (int i = 0; i < liveChannels.size(); i++)
		anyChannelsEnabled |= liveChannels[i].enabled;

	if (toggleButton("Start", !streamThreadRunning && !shouldAbortStreamThread && communication::isConnected())) {
		if(livePollThread.joinable()){
			livePollThread.join();
		}
		livePollThread = thread(livePollThreadFunc);
	}
	SameLine();
	if (toggleButton("Stop", streamThreadRunning && !shouldAbortStreamThread)) {
		abortStreamThread();
	}
	SameLine();
	if (toggleButton("Clear Data", !streamThreadRunning)) {
		for (int i = 0; i < liveChannels.size(); i++)
			resetDataChannelData(liveChannels[i]);
	}
	Checkbox("Auto scroll", &autoScroll);
	PushItemWidth(60);
	SameLine();
	InputFloat("time range", &LineGraph::autoScrollTimeRange);
	LineGraph::autoScrollTimeRange = glm::clamp(LineGraph::autoScrollTimeRange, 1.0f ,60.0f);
	PopItemWidth();
	Separator();

	for (int i = 0; i < liveChannels.size(); i++)
	{
		Text("Channel %d", i);
		PushID(i);

		SameLine(GetWindowWidth() - 190);
		if(Button("Export")){
			fileDialogHandler = saveDataFile;
			// This seems unsafe since it's from a vector..
			channelSaveTarget = &liveChannels[i];
			saveFileDialog(fileDialogFilenameBuffer);
		}
		SameLine(GetWindowWidth() - 130);
		if (Button("Delete channel")) {
			liveChannels.erase(liveChannels.begin() + i);
			PopID();
			continue;
		}

		PushItemWidth(60);
		InputInt("Graph ID", &liveChannels[i].graphID);
		Checkbox("Enabled", &liveChannels[i].enabled);
		SameLine();
		Checkbox("Visible", &liveChannels[i].visible);
		liveChannels[i].graphID = glm::clamp(liveChannels[i].graphID, 0, maxGraphs);
		PopItemWidth();
		InputText("Setting", liveChannels[i].settingName, IM_ARRAYSIZE(liveChannels[i].settingName));
		PushStyleColor(ImGuiCol_Header, ImVec4(liveChannels[i].color[0], liveChannels[i].color[1], liveChannels[i].color[2], 1.0f));
		Checkbox("Axis Scope", &liveChannels[i].axisScopeEnabled);
		SameLine();
		PushItemWidth(60);
		InputInt("Axis",  &liveChannels[i].axis);		
		PopItemWidth();
		liveChannels[i].axis = glm::clamp(liveChannels[i].axis, 1, 6);

		// The text on the color picker is sometimes unreadable. Change the text color to have higher contrast
		float brightness = liveChannels[i].color[0] * 0.2126f + liveChannels[i].color[1] * 0.7152f + liveChannels[i].color[2] * 0.0722f;
		if(brightness < 0.6f)
			PushStyleColor(ImGuiCol_Text, (ImVec4)white);
		else
			PushStyleColor(ImGuiCol_Text, (ImVec4)black);

		bool showColorPicker =  CollapsingHeader("Color picker");
		PopStyleColor(2);
		if(showColorPicker)
			ColorPicker3("Line color", liveChannels[i].color);		
		
		PopID();
		ImGui::Separator();
	}
	if (Button("Add new channel")){
		liveChannels.push_back(dataChannel(false));
		if(darkmode){
			liveChannels[liveChannels.size()-1].color[0] = defaultDarkModeLineColor[0];
			liveChannels[liveChannels.size()-1].color[1] = defaultDarkModeLineColor[1];
			liveChannels[liveChannels.size()-1].color[2] = defaultDarkModeLineColor[2];
		}
		else{
			liveChannels[liveChannels.size()-1].color[0] = defaultLightModeLineColor[0];
			liveChannels[liveChannels.size()-1].color[1] = defaultLightModeLineColor[1];
			liveChannels[liveChannels.size()-1].color[2] = defaultLightModeLineColor[2];
		}
	}
}

const unsigned int maxStartScriptLength = 10; // Could be any number.
char scopeStartScript[maxStartScriptLength][254];
bool scopeRunning = false;
bool dualAxisScope = false;
thread scopeThread;
volatile float scopeProgress = 0;
atomic_bool manualStopScope(false);

void scopeThreadFunc(){
	using namespace ImGui;
	using namespace communication;

	scopeProgress = 0;
	scopeRunning = true;

		sendAndWait("/scope clear\n");

		char scopeSettingBuff[256];
		sprintf(scopeSettingBuff, "/0 set scope.timebase %g\n", timeBase);
		sendAndWait(scopeSettingBuff);
		sprintf(scopeSettingBuff, "/0 set scope.delay %g\n", scopeDelay);
		sendAndWait(scopeSettingBuff);

		int localActiveChannels = 0;
		for (int i = 0; i < numScopeChannels; i+= dualAxisScope ? 2 : 1)
		{
			if (!scopeChannels[i].enabled)
				continue;
			localActiveChannels++;

			char settingBuff[256];
			sprintf(settingBuff, "/0 /%d scope add %s\n", !dualAxisScope, scopeChannels[i].settingName);
			sendAndWait(settingBuff);
		}
		activeScopeChannels = localActiveChannels;


		// Send out commands from the pre-scope script
		for (unsigned int i = 0; i < maxStartScriptLength; i++)
		{
			if(strnlen(scopeStartScript[i], 254) > 1){
				char startCommandBuffer[256];
				sprintf(startCommandBuffer, "/%s\n", scopeStartScript[i]);
				sendAndWait(startCommandBuffer);
			}
		}

		sendAndWait("/scope start\n");

		float ellapsedMilliseconds = 0;
		float totalTime = 1024 * timeBase;
		while (true)
		{
			if (!sendAndWait("/scope print\n")){
				this_thread::sleep_for(std::chrono::milliseconds(10));
				ellapsedMilliseconds += 10;
			}
			else{
				break;
			}

			scopeProgress = ellapsedMilliseconds / totalTime;
			
			if(ellapsedMilliseconds > totalTime * 1.5){
				cout << "Timed out waiting for scope printout" << endl;
				return;
			}
			if(manualStopScope){
				manualStopScope = false;
				if(!sendAndWait("/scope stop\n")){
					cout << "Error manually stopping scope" << endl;
					return;
				}
				else if(!sendAndWait("/scope print\n")){
					cout << "Error manually stopping scope" << endl;
					return;
				}
				else
					break;
			}
		}
		scopeProgress = 1.0f;

		cout << "printing..." << endl;

		receive(reply);
		auto fields = stringSplit(string(reply), " ");
		int dataCount = stoi(fields[3]);
		int channelCount = stoi(fields[5]);

		scopeProgress = 0.0f;
		unsigned int expectedPoints = dataCount * channelCount;
		float progressIncrement = 1.0f / expectedPoints;
		for (int i = 0; i < channelCount && i < numScopeChannels; i++)
		{
			resetDataChannelData(scopeChannels[i]);
			//data header
			receive(reply);
			for (int j = 0; j < dataCount; j++)
			{
				receive(reply);
				auto dataFields = stringSplit(string(reply), " ");
				float dataPoint = stof(dataFields[3]);
				dataLock.lock();
				scopeChannels[i].dataPoints.push_back(dataPoint);
				scopeChannels[i].pointCount++;
				scopeChannels[i].timePoints.push_back((j * timeBase) / 1000.0 + scopeDelay);
				scopeChannels[i].minPoint = glm::min(scopeChannels[i].minPoint, dataPoint);
				scopeChannels[i].maxPoint = glm::max(scopeChannels[i].maxPoint, dataPoint);
				dataLock.unlock();
				scopeProgress += progressIncrement;
			}
			scopeChannels[i].timeBase = timeBase;
			updateDataChannelFields(scopeChannels[i]);
		}
		scopeRunning = false;
	scopeProgress = 0.0f;
}

void showScopeChannelTab() {
	using namespace ImGui;
	using namespace communication;

	bool anyChannelsEnabled = false;
	for (int i = 0; i < numScopeChannels; i++)
		anyChannelsEnabled |= scopeChannels[i].enabled;
	
	ProgressBar(scopeProgress);
	bool start = toggleButton("Start", anyChannelsEnabled && communication::isConnected() && !streamThreadRunning);
	SameLine();
	if(toggleButton("Manual stop", scopeRunning)){
		manualStopScope = true;
	}

	SameLine();
	if(Button("Edit Script")){
		scriptWindowOpen = true;
	}

	PushItemWidth(60);
	InputFloat("Timebase", &timeBase);
	timeBase = glm::max(timeBase, 0.1f);
	SameLine();
	InputFloat("Delay", &scopeDelay);
	scopeDelay == glm::max(scopeDelay, 0.0f);
	PopItemWidth();
	Separator();
	Checkbox("Two axis mode", &dualAxisScope);

	if (start) {
		if(scopeThread.joinable())
			scopeThread.detach();
		scopeThread = thread(scopeThreadFunc);
	}
	Separator();
	for (int i = 0; i < numScopeChannels; i++)
	{
		if (dualAxisScope){
			Text("Channel %d (axis %d)", i, i % 2 ? 2 : 1);
			PushID(i);
		}
		else{
			Text("Channel %d", i);
			PushID(i);
		}

		SameLine(GetWindowWidth() - 75);
		if(Button("Export")){
			fileDialogHandler = saveDataFile;
			// This seems unsafe since it's from a vector..
			channelSaveTarget = &scopeChannels[i];
			saveFileDialog(fileDialogFilenameBuffer);
		}

		PushItemWidth(60);
		InputInt("Graph ID", &scopeChannels[i].graphID);

		if(!dualAxisScope || !(i % 2)){
			Checkbox("Enabled", &scopeChannels[i].enabled);
			SameLine();
		}
		else
			scopeChannels[i].enabled = scopeChannels[i-1].enabled;
		
		Checkbox("Visible", &scopeChannels[i].visible);

		scopeChannels[i].graphID = glm::clamp(scopeChannels[i].graphID, 0, maxGraphs);
		PopItemWidth();

		if(dualAxisScope && i % 2)
			InputText("Setting", scopeChannels[i - 1].settingName, IM_ARRAYSIZE(scopeChannels[i -1].settingName), ImGuiInputTextFlags_ReadOnly);
		else
			InputText("Setting", scopeChannels[i].settingName, IM_ARRAYSIZE(scopeChannels[i].settingName));
		
		PushStyleColor(ImGuiCol_Header, ImVec4(scopeChannels[i].color[0], scopeChannels[i].color[1], scopeChannels[i].color[2], 1.0f));

		// The text on the color picker is sometimes unreadable. Change the text color to have higher contrast
		float brightness = scopeChannels[i].color[0] * 0.2126f + scopeChannels[i].color[1] * 0.7152f + scopeChannels[i].color[2] * 0.0722f;
		if(brightness < 0.6f)
			PushStyleColor(ImGuiCol_Text, (ImVec4)white);
		else
			PushStyleColor(ImGuiCol_Text, (ImVec4)black);

		bool showColorPicker =  CollapsingHeader("Color picker");
		PopStyleColor(2);
		if(showColorPicker)
			ColorPicker3("Line color", scopeChannels[i].color);

		PopID();
		ImGui::Separator();
	}
}

void saveDataFile(){
	cout << "saving some data to: " << string(fileDialogFilenameBuffer) << endl;

	if(channelSaveTarget == nullptr){
		printf("Channel save target was null somehow at line number %d in file %s\n", __LINE__, __FILE__);
		return;
	}

	writeChannelData(fileDialogFilenameBuffer, *channelSaveTarget, TimeUnits::SECONDS, DataUnits::RAW);	

	channelSaveTarget = nullptr;
}
void loadDataFile(){
	cout << "loading some data from: " << string(fileDialogFilenameBuffer) << endl;
	dataChannel newChannel;
	readChannelData(fileDialogFilenameBuffer, &newChannel);
	liveChannels.push_back(newChannel);
}

// Window to privide text input for pre-scope scripting
void showScriptEditWindow(){
	using namespace ImGui;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoDocking;
	Begin("Scope start script", &scriptWindowOpen, window_flags);

	Text("These commands will be sent to the device immediately before scope start is sent.");
	Separator();

	for (unsigned int i = 0; i < maxStartScriptLength; i++)
	{
		char id[8];
		sprintf(id, "text%d", i);
		PushID(id);
		Text("/");
		SameLine();
		InputText("\\n", scopeStartScript[i], 254);
		PopID();
	}
	End();
}

ImGuiTextBuffer consoleBuf;
bool ScrollToBottom;
char consoleInputBuf[256];
void showCommandWindow(){
	using namespace ImGui;

	ImGuiWindowFlags window_flags = 0;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoDocking;
	Begin("Console", &commandWindowOpen, window_flags);

	bool enterKey = InputText("", consoleInputBuf, IM_ARRAYSIZE(consoleInputBuf), ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine();
	if(toggleButton("Send", communication::isConnected()) || (enterKey && communication::isConnected())){
		if(strlen(consoleInputBuf) > 0){
			if(streamThreadRunning){
				commanInterjections.enqueue(std::string(consoleInputBuf));
			}
			else{
				communication::sendAndWait(consoleInputBuf);
			}
		}
		memset(consoleInputBuf, 0, 256);
	}
	SameLine();
	if (ImGui::Button("Clear"))
		consoleBuf.clear();
	ImGui::Separator();
	ImGui::BeginChild("scrolling");
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

	ImGui::TextUnformatted(consoleBuf.begin());
	if (ScrollToBottom)
		ImGui::SetScrollHere(1.0f);
	ScrollToBottom = false;
	ImGui::PopStyleVar();
	ImGui::EndChild();
	//ImGui::End();
	End();
};

// Adds text to the command/console window.
void AddLog(const char *fmt, ...)
{
	using namespace ImGui;
	va_list args;
	va_start(args, fmt);
	consoleBuf.appendfv(fmt, args);
	va_end(args);
	ScrollToBottom = true;
}

// callback function from communication.h
void onReceive(char data[256]){
	AddLog("%s\n", data);
}

void showMainMenueBar() {
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("File menu", NULL, false, false);
			if (ImGui::MenuItem("Load data", "Ctrl+O")) {
				fileDialogHandler = loadDataFile;
				openFileDialog(fileDialogFilenameBuffer);
			}
			
			// Check box for switching theme. When switched, toggle imgui theme, then
			// for any channel colors that are still left as default, switch to a color which
			// will be visible with the new theme.
			if(ImGui::Checkbox("Dark mode", &darkmode)){
				if(darkmode){
					ImGui::StyleColorsDark();
					for (size_t i = 0; i < numScopeChannels; i++)
					{
						if
						(
							scopeChannels[i].color[0] == defaultLightModeLineColor[0] &&
							scopeChannels[i].color[1] == defaultLightModeLineColor[1] &&
							scopeChannels[i].color[2] == defaultLightModeLineColor[2]
						){
							scopeChannels[i].color[0] = defaultDarkModeLineColor[0];
							scopeChannels[i].color[1] = defaultDarkModeLineColor[1];
							scopeChannels[i].color[2] = defaultDarkModeLineColor[2];
						}
					}
					for (size_t i = 0; i < liveChannels.size(); i++)
					{
						if
						(
							liveChannels[i].color[0] == defaultLightModeLineColor[0] &&
							liveChannels[i].color[1] == defaultLightModeLineColor[1] &&
							liveChannels[i].color[2] == defaultLightModeLineColor[2]
						){
							liveChannels[i].color[0] = defaultDarkModeLineColor[0];
							liveChannels[i].color[1] = defaultDarkModeLineColor[1];
							liveChannels[i].color[2] = defaultDarkModeLineColor[2];
						}
					}
					
				}
				else{
					ImGui::StyleColorsLight();
					for (size_t i = 0; i < numScopeChannels; i++)
					{
						if
						(
							scopeChannels[i].color[0] == defaultDarkModeLineColor[0] &&
							scopeChannels[i].color[1] == defaultDarkModeLineColor[1] &&
							scopeChannels[i].color[2] == defaultDarkModeLineColor[2]
						){
							scopeChannels[i].color[0] = defaultLightModeLineColor[0];
							scopeChannels[i].color[1] = defaultLightModeLineColor[1];
							scopeChannels[i].color[2] = defaultLightModeLineColor[2];
						}
					}
					for (size_t i = 0; i < liveChannels.size(); i++)
					{
						if
						(
							liveChannels[i].color[0] == defaultDarkModeLineColor[0] &&
							liveChannels[i].color[1] == defaultDarkModeLineColor[1] &&
							liveChannels[i].color[2] == defaultDarkModeLineColor[2]
						){
							liveChannels[i].color[0] = defaultLightModeLineColor[0];
							liveChannels[i].color[1] = defaultLightModeLineColor[1];
							liveChannels[i].color[2] = defaultLightModeLineColor[2];
						}
					}
				}
			}
			

			ImGui::Separator();
			if (ImGui::MenuItem("Quit", "Alt+F4")) {
				glfwSetWindowShouldClose(window, true);
			}
			ImGui::EndMenu();
		}
		// if (ImGui::BeginMenu("Edit"))
		// {
		// 	if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
		// 	if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
		// 	ImGui::Separator();
		// 	if (ImGui::MenuItem("Cut", "CTRL+X")) {}
		// 	if (ImGui::MenuItem("Copy", "CTRL+C")) {}
		// 	if (ImGui::MenuItem("Paste", "CTRL+V")) {}
		// 	ImGui::EndMenu();
		// }
		ImGui::EndMainMenuBar();
	}
}