#pragma once

#include <glm/glm.hpp> 
#include "line_graph.h"

enum class TimeUnits{
    SECONDS
};
enum class DataUnits{
    RAW,
    SCIENTIFIC
};

// Exports channel data to a file. Returns success
bool writeChannelData(const char * filename, dataChannel &channel, TimeUnits timeUnits, DataUnits dataUnits);

// Imports channel data from a file. Returns success
bool readChannelData(const char * filename, dataChannel * channel);