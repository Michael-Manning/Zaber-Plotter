#pragma once

#include <iomanip>
#include <string>
#include <vector>
#include <atomic>

#include <glm/glm.hpp> //math
#include <imgui/imgui.h>

#include "configuration.h"

const float defaultDarkModeLineColor[3]{ 0.9f, 0.9f,1.0f };
const float defaultLightModeLineColor[3]{ 0.0f, 0.0f, 0.3f };

// Data which can be graphed
struct dataChannel {

	// Wether this channel is being used for a standard scope channel or for graphing
	// arbitrary data such as live streaming which ignors the timebase.
	bool scopeMode = true;

	std::vector<float> dataPoints;
	std::vector<double> timePoints;
	unsigned int pointCount = 0;
	float timeBase = 0.1;
	float color[3]{ 0.9f, 0.9f,1.0f };
	bool enabled = false;
	bool visible = false;
	char settingName[128] = "encoder.pos";
	float minPoint = 0.0f;
	float maxPoint = 0.0f;
	int graphID = 0;
	int graphSide = 0;

	// Only used by live channels.
	bool axisScopeEnabled = false;
	int axis = 0;

	//reserve max data points for a standard scope channel
	dataChannel(bool isScopeChannel = true){
		if(isScopeChannel){
			//dataPoints.reserve(1024);
			//timePoints.reserve(1024);
			scopeMode = true;
		}
	}
};

// Clears add the data and time points from a channel. NOT THREAD SAFE.
void resetDataChannelData(dataChannel &channel);

// Updates min and max data/time fields and pointcount fields
void updateDataChannelFields(dataChannel &channel);

class LineGraph {
public:

    // Settings
    static float zoomSensitivity;
    const float verticalLockPadding = 30; //Pixel padding between line points and vertical graph edges
    const float horizontalLockPadding = 50; //Pixel padding between line points and vertical graph edges
    constexpr static float minZoom = 1.0f;
    const float maxZoom = 13.0f;
    const float zoomDetailThreshold = 5.0f; // Zoom level in which details are rendered (eg. red circles on data points) 
	const bool useGPU = true;
	static float autoScrollTimeRange;


    glm::vec2 prevMousePos = glm::vec2(0);
	glm::vec2 mouseDelta = glm::vec2(0);
	static float canvasPanX;
	float canvasPanY = 0.0f;
	glm::vec2 effectiveCanvasPan = glm::vec2(0);

	static ImFont* labelFont;

    void render(std::vector<dataChannel> &channels, float zoomDelta, float height, bool darkTheme, bool autoScroll);
	static void resetHorizontalScale(){
		horizontalZoom = 1.0f;
	};

private:
	glm::vec2 zoomPoint;
	float verticalZoom = 1.0;
	static float horizontalZoom;
    float horizontalScale = 100.0f;
	bool dragging = false;
	int interpolationTargetIndex = -1;
};