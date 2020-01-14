#include "line_graph.h"

#include <iomanip>
#include <string>
#include <vector>
#include <iostream>

#include <imgui/imgui.h>
#include <glm/glm.hpp> //math

#include "configuration.h"
#include "gui_utils.h"
#include "assert.h"


#ifdef _WIN32
#undef min
#undef max
#include <cfloat>
#define MAXFLOAT FLT_MAX
#endif

using namespace std;
using namespace glm;
using namespace ImGui;

// Basic colors
#define white (ImColor)IM_COL32(255, 255, 255, 255)
#define red (ImColor)IM_COL32(255, 0, 0, 255)
#define black (ImColor)IM_COL32(0, 0, 0, 255)
#define orange (ImColor)IM_COL32(239, 136, 95, 255)
#define blue (ImColor)IM_COL32(25, 104, 232, 255)
#define yellow (ImColor)IM_COL32(255, 255, 0, 255)
#define translucent ImColor)IM_COL32(255, 255, 255, 100)
#define invisible (ImColor)IM_COL32(0, 0, 0, 0)

// Bounding box check.
inline bool within(vec2 A, vec2 B, vec2 point) {
	return
		point.x >= A.x &&
		point.x <= B.x &&
		point.y >= A.y &&
		point.y <= B.y
		;
}

// Point between two points.
vec2 getLinearPoint(float t, vec2 a, vec2 b) {
	//https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Linear_B%C3%A9zier_curves
	return a + t * (b - a);
}

void resetDataChannelData(dataChannel &channel){
	channel.timePoints.clear();
	channel.dataPoints.clear();
	channel.minPoint = 0;
	channel.maxPoint = 0;
	channel.pointCount = 0;
}

void updateDataChannelFields(dataChannel &channel){
	int pointCount = channel.dataPoints.size();
	assert(channel.timePoints.size() == pointCount);
	channel.pointCount = pointCount;
	
	if(pointCount == 0)
		return;

	channel.minPoint = channel.dataPoints[0];
	channel.maxPoint = channel.dataPoints[0];

	for (unsigned int i = 1; i < pointCount; i++)
	{
		channel.minPoint = glm::min(channel.dataPoints[i], channel.minPoint);
		channel.maxPoint = glm::max(channel.dataPoints[i], channel.maxPoint);	
	}
}

float LineGraph::zoomSensitivity = 0.2f;
float LineGraph::autoScrollTimeRange = 7.0f;
float LineGraph::canvasPanX = 0.0f;
float LineGraph::horizontalZoom = 1.0f;
ImFont * LineGraph::labelFont = nullptr;

// Transforms a point on a graph to account for zooming and panning 
inline vec2 canvasPointTranform(vec2 point, vec2 canPos, vec2 pointOffset, vec2 pan, vec2 zoom, vec2 zoomPoint) {
	point += pointOffset;
	point -= zoomPoint;
	point *= zoom;
	point += zoomPoint;
	point += pan;
	return point;
}

// Gets the would-be time and data value from a point on the graph in pixels   
inline vec2 graphPointTranform(vec2 point, vec2 pointOffset, vec2 pan, vec2 zoom, vec2 zoomPoint) {
	point -= pan;
	point -= zoomPoint;
	point /= zoom;
	point += zoomPoint;
	point -= pointOffset;
	return point;
}

void LineGraph::render(vector<dataChannel> &channels, float zoomDelta, float height, bool darkTheme, bool autoScroll){

    ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	vec2 canPos = ImGui::GetCursorScreenPos();

	vec2 canvasSize = ImGui::GetContentRegionAvail();
	if (canvasSize.x < 50.0f) canvasSize.x = 50.0f;
	if (canvasSize.y < 50.0f) canvasSize.y = 50.0f;

	// Add graph borders and clipping rect.
	if(darkTheme){
		drawList->AddRectFilled(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), IM_COL32(25, 25, 30, 255));
		drawList->AddRect(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), white);
		drawList->PushClipRect(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), true);
	}
	else
	{
		drawList->AddRectFilled(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), IM_COL32(255, 255, 255, 255));
		drawList->AddRect(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), black);
		drawList->PushClipRect(canPos, ImVec2(canPos.x + canvasSize.x, canPos.y + canvasSize.y), true);
	}

	// Give the graph a unique ID so imgui doesn't mix it up
	char canvasID[9];
	sprintf(canvasID, "canvas%d", channels[0].graphID);

	vec2 mousePos = ImVec2(ImGui::GetIO().MousePos.x - canPos.x, ImGui::GetIO().MousePos.y - canPos.y);

	// Don't continue if there is no data.
    bool someThingToDraw = false;
	for (int i = 0; i < channels.size(); i++)
		if(channels[i].dataPoints.size() > 1)
			someThingToDraw = true;
	if(!someThingToDraw){
		drawList->PopClipRect();
		return;
	}

    // Mouse is hovering on canvas.
    bool hoveringOnCanvas = (
		mousePos.x > 0 &&
		mousePos.y > 0 &&
		mousePos.x < canvasSize.x  &&
		mousePos.y < canvasSize.y
	);
    
	// Horizontal zooming.
    bool altZoom = io.KeyCtrl;
	bool zoomChanged = false;

    //Increment and clamp zoom amount.
    if(hoveringOnCanvas &&  zoomDelta != 0.0f){
        if(altZoom){
			horizontalZoom += zoomDelta * zoomSensitivity;   
        }
        else{
			float prevY = verticalZoom;
            verticalZoom +=  zoomDelta * zoomSensitivity;
			verticalZoom = glm::clamp(verticalZoom, minZoom, maxZoom);

			float scaleFact = verticalZoom / prevY;
			horizontalZoom *= scaleFact;

            zoomChanged = true;
        }
		horizontalZoom = glm::max(horizontalZoom, minZoom);
    }

	// Convenience.
	vec2 zoomAmount(horizontalZoom, verticalZoom);

	// Panning
	vec2 delta = prevMousePos - mousePos;
	prevMousePos = mousePos;
	// Only allow dragging when hovering, but allow continuation of drag when not hovering.
	if (IsMouseClicked(1) && hoveringOnCanvas){
		delta = vec2(0);
		dragging = true;
	}
	if (IsMouseDown(1)) {
		if (dragging)
		{
			canvasPanX -= delta.x / zoomAmount.x;
			canvasPanY -= delta.y / zoomAmount.y;
		}
	}
	else
		dragging = false;


    // Get absolute range of all data
    float minData = channels[0].minPoint;
    float maxData = minData;
	float minTime = channels[0].timePoints[0];
	float maxTime = minTime;
    for(int i = 0; i < channels.size(); i++){
		minData = glm::min(minData, channels[i].minPoint);
        maxData = glm::max(maxData, channels[i].maxPoint);
    }
	for(int i = 0; i < channels.size(); i++){
		for (int j = 0; j < channels[i].timePoints.size(); j++){
			minTime = glm::min(minTime, (float)channels[i].timePoints[j]);
			maxTime = glm::max(maxTime, (float)channels[i].timePoints[j]);
		}
	}
	float dataRange = maxData - minData;
	if(dataRange == 0){
		maxData ++;
		minData --;
		dataRange = 2.0f;
	}
	
	// I know this is strange, but there's pretty much zero benifit to displaying graphs that don't start at zero.
	// By fixing the min time like this it's possible to display live graphs that were enabled at different times.
	minTime = 0.0f;

	float timeRange = maxTime - minTime;

	// Get scale factors to apply to actual data points before applying navigation transformations.
	// This Makes all data fit nicely within the graph size in pixels to reduce complexity,
	vec2 dataScaler;
	dataScaler.x = (canvasSize.x - horizontalLockPadding * 2) / timeRange;
	dataScaler.y = (canvasSize.y - verticalLockPadding * 2) / dataRange;
	
	// The location to zoom into when scrolling. Set to the centre of the canvas for now, but it could just as easily
	// by the mouse position.
	zoomPoint = canPos + canvasSize / 2.0f;
	
	// Offset all points by this vector.
	vec2 offset = vec2(0, minData * dataScaler.y);

	// When autoscrolling, fix the horizontal zoom and then just slam the panning amount a ridiculous amount to the right.
	// The panning lock will take care of fixing it the right edge. 
	if(autoScroll && timeRange > autoScrollTimeRange){
		horizontalZoom = timeRange / autoScrollTimeRange;
		zoomAmount.x = horizontalZoom;
		canvasPanX = -999999999999;
	}
	
	effectiveCanvasPan = vec2(canvasPanX, canvasPanY) * zoomAmount;

	//vertical panning lock
	{

		float max = canPos.y +verticalLockPadding + zoomPoint.y * (zoomAmount.y - 1.0f) + dataScaler.y * -minData * 2 * zoomAmount.y;
		float min = max + -((canvasSize.y - verticalLockPadding * 2)* (zoomAmount.y - 1.0f));

		canvasPanY = glm::clamp(canvasPanY, min / zoomAmount.y, max / zoomAmount.y);
		effectiveCanvasPan.y = glm::clamp(effectiveCanvasPan.y, min, max);
	}

	//horizontal panning lock
	{
		float max = canPos.x + horizontalLockPadding + (zoomPoint.x * (zoomAmount.x - 1.0f));
		float min = max + -((canvasSize.x - horizontalLockPadding * 2)* (zoomAmount.x - 1.0f));

	    canvasPanX = glm::clamp(canvasPanX, min / zoomAmount.x, max / zoomAmount.x);
		effectiveCanvasPan.x = glm::clamp(effectiveCanvasPan.x, min, max);
	}

	// label data
	int verticalLabelPointsCount = 0;
	const int maxVerticalPoints = 10;
	const int labelTextHeight = 15;
	const int minLabelSpacing = 50;

	float verticalLabelPoints[maxVerticalPoints + 1];;
	{
		// Figure out the data range that is actually visible on the graph right now 
		float minGraphPoint = canvasSize.y + canPos.y;
		minGraphPoint = graphPointTranform(vec2(0, minGraphPoint), offset, effectiveCanvasPan, zoomAmount, zoomPoint).y;
		minGraphPoint = -((minGraphPoint / dataScaler.y) - minData - maxData);
		float maxGraphPoint = canPos.y;
		maxGraphPoint = graphPointTranform(vec2(0, maxGraphPoint), offset, effectiveCanvasPan, zoomAmount, zoomPoint).y;
		maxGraphPoint = -((maxGraphPoint / dataScaler.y) - minData - maxData);

		float visibleDataRange = maxGraphPoint - minGraphPoint;
		
		// Decide what increment to use for the labels. Factors of 1, 2, and 5 are standard.
		// We divide the power by a lot at first in case the data range is less than zero.  
		float increment = 0;
		{
			int power = 0;
			const int bases[] = {1, 2, 5};
			float powerDivisor = 100000;
			while (power < 9999)
			{
				for (size_t i = 0; i < 3; i++)
				{
					increment = bases[i] * pow(10, power) / powerDivisor;
					float projectedCount = visibleDataRange / increment;
					if (projectedCount <= maxVerticalPoints  && canvasSize.y / projectedCount >= minLabelSpacing)
						goto doublebreakV;
				}
				power++;
			}
		}
doublebreakV:;
		verticalLabelPointsCount = (int)ceil(visibleDataRange / increment);

		int bottomMultiplier = floor(minGraphPoint / increment);
		while(bottomMultiplier * increment < minGraphPoint)
			bottomMultiplier ++;
		for (int i = 0; i < verticalLabelPointsCount + 1; i++)
		{
			verticalLabelPoints[i] = (i + bottomMultiplier) * increment;
		}
		
	}

	int horizontalLabelPointsCount = 12;
	const int maxHorizontalPoints = 10;
	float horizontalLabelPoints[maxHorizontalPoints + 1];
	float minVisibleTimePoint, maxVisibleTimePoint; // Also used to skip draw calls later on.

	{
		minVisibleTimePoint =  canPos.x;
		minVisibleTimePoint = graphPointTranform(vec2(minVisibleTimePoint, 0), offset, effectiveCanvasPan, zoomAmount, zoomPoint).x;
		minVisibleTimePoint = (minVisibleTimePoint / dataScaler.x);
		maxVisibleTimePoint = canvasSize.x + canPos.x;
		maxVisibleTimePoint = graphPointTranform(vec2(maxVisibleTimePoint, 0), offset, effectiveCanvasPan, zoomAmount, zoomPoint).x;
		maxVisibleTimePoint = (maxVisibleTimePoint / dataScaler.x);

		float visibleDataRange = maxVisibleTimePoint - minVisibleTimePoint;

		float increment = 0;
		{
			int power = 0;
			const int bases[] = {1, 2, 5};
			float powerDivisor = 100000;
			while (power < 9999)
			{
				for (size_t i = 0; i < 3; i++)
				{
					increment = bases[i] * pow(10, power) / powerDivisor;
					if (visibleDataRange / increment <= maxHorizontalPoints)
						goto doublebreakH;
				}
				power++;
			}
		}
doublebreakH:;
		horizontalLabelPointsCount = (int)ceil(visibleDataRange / increment);

		int bottomMultiplier = floor(minVisibleTimePoint / increment);
		while(bottomMultiplier * increment < minVisibleTimePoint)
			bottomMultiplier ++;
		for (int i = 0; i < horizontalLabelPointsCount + 1; i++)
		{
			horizontalLabelPoints[i] = (i + bottomMultiplier) * increment;
		}
		
	}

	// Drawing labels.
	char labelBuff[256];
	{
		for (int i = 0; i < verticalLabelPointsCount + 1; i++)
		{
			vec2 transformed = canvasPointTranform(vec2(0, (maxData - verticalLabelPoints[i] + minData) * dataScaler.y), canPos, offset, effectiveCanvasPan, zoomAmount, zoomPoint);
			sprintf(labelBuff, "%g", verticalLabelPoints[i]);
			drawList->AddText(labelFont, labelTextHeight, vec2(canPos.x + 15, transformed.y), darkTheme ? white : black, labelBuff);
		}

		for (int i = 0; i < horizontalLabelPointsCount + 1; i++){
			sprintf(labelBuff, "%gs", horizontalLabelPoints[i]);
			vec2 transformed = canvasPointTranform(vec2(horizontalLabelPoints[i] * dataScaler.x, 0), canPos, offset, effectiveCanvasPan, zoomAmount, zoomPoint);
			// Text offset to the right slightly to make room for tick markers.
			drawList->AddText(labelFont, labelTextHeight, vec2(transformed.x + 6, canPos.y + canvasSize.y - 20), darkTheme ? white : black, labelBuff);

		}
	}

	//grid lines
	{
		for (int i = 0; i < verticalLabelPointsCount + 1; i++)
		{
			vec2 transformed = canvasPointTranform(vec2(0, (maxData - verticalLabelPoints[i] + minData) * dataScaler.y), canPos, offset, effectiveCanvasPan, zoomAmount, zoomPoint);

			float lineWidth = verticalLabelPoints[i] == 0 ? 4: 1;
			uint8 opacity = 50;
			
			auto lineCol = darkTheme ? IM_COL32(255, 190, 150, opacity) : IM_COL32(0, 0, 0, opacity);
			drawList->AddLine(vec2(canPos.x, transformed.y), vec2(canvasSize.x + canPos.x, transformed.y), lineCol, lineWidth);
		}

		for (int i = 0; i < horizontalLabelPointsCount + 1; i++)
		{
			vec2 transformed = canvasPointTranform(vec2(horizontalLabelPoints[i] * dataScaler.x, 0), canPos, offset, effectiveCanvasPan, zoomAmount, zoomPoint);
			vec2 p1(transformed.x, canPos.y + canvasSize.y);
			
			float lineWidth = 1;
			uint8 opacity = 50;

			auto lineCol = darkTheme ? IM_COL32(255, 190, 150, opacity) : IM_COL32(0, 0, 0, opacity);
			drawList->AddLine(p1, p1 - vec2(0, canvasSize.y), lineCol, lineWidth);
			//drawList->AddLine(p1, p1 + vec2(0, -20), IM_COL32(255, 255, 255, 255), 1);
		}
	}


	// For figuring out which channel's points are closest to the mouse.
	int closestPointToMouseChannelIndex = 0;
	float closestPointToMouseDistance = MAXFLOAT;

	// For figuring out which individual points are being interpolated.
    float pointNearMouseXA = MAXFLOAT;
	int pointNearMouseXAIndex = 0;
	vec2 pointNearMouseAPos = vec2();
	float pointNearMouseXB = MAXFLOAT;
	int pointNearMouseXBIndex = 0;
	vec2 pointNearMouseBPos = vec2();

	// Reset interpolation if shift key is released
	if(!io.KeyShift){
		interpolationTargetIndex = -1;
	}

	//lines and circles
	bool toolTipShown = false;
	bool prevInView = true;
	pointNearMouseXA = MAXFLOAT;
	pointNearMouseXB = MAXFLOAT;

    for (int chanI = 0; chanI < channels.size(); chanI++)
    {
        dataChannel channel =channels[chanI];
        vector<float> dataBuffer = channel.dataPoints;
        vector<double> timeBuffer = channel.timePoints;
        auto lineCol = IM_COL32(channel.color[0] * 255, channel.color[1] * 255, channel.color[2] * 255, 255);
		vec2 prev(0, 999);

		// Can't draw less that 2 points.
		if(dataBuffer.size() < 2)
			continue;

		// Drawing all data.
        for (int i = 0; i < channel.dataPoints.size(); i++)
        {
			// No way to really calculate the first visible index since live graphs don't have evenly
			// space time intervels between data points.
			if(i < timeBuffer.size() - 1 && timeBuffer[i+1] < minVisibleTimePoint){
				continue;
			}
			//Nothing else will be visible, just quit now.
			if(i > 0 && timeBuffer[i-1] > maxVisibleTimePoint){
				break;
			}

			// Y axis is positive in the downward direction
			dataBuffer[i] = maxData - dataBuffer[i] + minData;
			// Get next point scaled to fit into graph size pixels.
            vec2 next((float)timeBuffer[i] * dataScaler.x, dataBuffer[i] * dataScaler.y);
			// Apply navigation tranformation.
            next = canvasPointTranform(next, canPos, offset, effectiveCanvasPan, zoomAmount, zoomPoint);

			// Draw lines, circles, and hovering tooltip. 
            if ((prevInView) && i > 0) {
                drawList->AddLine(prev, next, lineCol , 2);
				//packedLines[i] = packedLine(prev - canPos, next - canPos);
                if (zoomAmount.y > zoomDetailThreshold) {
					// Mouse is hovering on top of point.
                    if (!toolTipShown && distance(mousePos + canPos, next) < 8)
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("value: %f\nTime: %f", maxData - dataBuffer[i] + minData  , timeBuffer[i]);
                        ImGui::EndTooltip();
                        drawList->AddCircle(next, 8, yellow, 12, 3);
                        toolTipShown = true;
                    }
                    else
                        drawList->AddCircle(next, 8, red, 12, 3);
                }
            }

			// Detecting which points to interpolate between for the tooltip is done while they are being drawn.
			// This is to avoid applying the navigation transformation multiple times per point.
			if(io.KeyShift && hoveringOnCanvas){
				// Avoid square root if possible
				if(interpolationTargetIndex == -1){
					float mousePointDist = glm::distance(mousePos + canPos, next);
					if (mousePointDist < closestPointToMouseDistance)
					{
						closestPointToMouseDistance = mousePointDist;
						closestPointToMouseChannelIndex = chanI;
					}
				} 
				// Checking the individual nearest horizontal point to the mouse.
				// This always lags one frame behind when the shift key is pressed to avoid duplicate tranformation calculations.
				if(interpolationTargetIndex == chanI){			
					// Figure out selected graph and points for interpolating by finding the two closest point to the mouse horizontally.
					float mousePointDistX = abs((mousePos.x + canPos.x) - next.x);
					if (mousePointDistX < pointNearMouseXA) {
						pointNearMouseXB = pointNearMouseXA;
						pointNearMouseXBIndex = pointNearMouseXAIndex;
						pointNearMouseBPos = pointNearMouseAPos;

						pointNearMouseXA = mousePointDistX;
						pointNearMouseXAIndex = i;
						pointNearMouseAPos = next;
					}
					else if (mousePointDistX < pointNearMouseXB) {
						pointNearMouseXB = mousePointDistX;
						pointNearMouseXBIndex = i;
						pointNearMouseBPos = next;
					}
				}

			}
            prev = next;
			// End of point loop.
        }
		// End of channel loop.
    }
    
	if(interpolationTargetIndex == -1 && hoveringOnCanvas && io.KeyShift){
		interpolationTargetIndex = closestPointToMouseChannelIndex;
	}

	if (hoveringOnCanvas && io.KeyShift && !toolTipShown) {
		// Enable this code block to display lines to the two points being used for linear interpolation.  
#if 0
		drawList->AddLine(mpos + canPos, nearMouseAPos, yellow);
		drawList->AddLine(mpos + canPos, nearMouseBPos, red);
#endif

		// For convenience, assign near mouse points to which is larger and smaller.
		vec2 bigger, smaller;
		int smallerIndex, biggerIndex;

		if (pointNearMouseAPos.x > pointNearMouseBPos.x) {
			bigger = pointNearMouseAPos;
			smaller = pointNearMouseBPos;
			biggerIndex = pointNearMouseXAIndex;
			smallerIndex = pointNearMouseXBIndex;
		}
		else {
			bigger = pointNearMouseBPos;
			smaller = pointNearMouseAPos;
			biggerIndex = pointNearMouseXBIndex;
			smallerIndex = pointNearMouseXAIndex;
		}

		// Calculate where along the line the mouse lines up between the left and right
		// point based only on the horizontal alignment.
		float t;
		{
			float lX = smaller.x;
			float rX = bigger.x;
			float mX = mousePos.x + canPos.x;

			rX -= lX;
			mX -= lX;
			lX = 0;

			t = mX / rX;
		}

		// Get the point along the line from both points using the calculated value.
		vec2 midPoint = getLinearPoint(t, smaller, bigger);

		drawList->AddLine(mousePos + canPos, midPoint, darkTheme ? white : black);

		auto dataBuffer = channels[interpolationTargetIndex].dataPoints;
		auto timeBuffer = channels[interpolationTargetIndex].timePoints;

		// Line interpolation: a + t * (b - a)
		float midData = dataBuffer[smallerIndex] + t * (dataBuffer[biggerIndex] - dataBuffer[smallerIndex]);
		double midTime = timeBuffer[smallerIndex] + t * (timeBuffer[biggerIndex] - timeBuffer[smallerIndex]);

		ImGui::BeginTooltip();
		Text("value: %f\nTime: %f", midData, midTime);
		ImGui::EndTooltip();
	}
	drawList->PopClipRect();
}