#include <glm/glm.hpp>
#include <vector>
#include <iostream>
#include <fstream>

#include "file_handling.h"

using namespace std;
using namespace glm;


bool writeChannelData(const char * filename, dataChannel &channel, TimeUnits timeUnits, DataUnits dataUnits){
    ofstream file;
    file.open(filename);
    file << "Time (s), " << string(channel.settingName) << "\n";
    for (int i = 0; i < channel.pointCount; i++)
    {
        if(dataUnits == DataUnits::RAW)
            file << std::fixed << channel.timePoints[i] << "," << std::fixed << channel.dataPoints[i] << "\n";
        else
            file << channel.timePoints[i] << "," << channel.dataPoints[i] << "\n";
    }
    file.close();
}

bool readChannelData(const char * filename, dataChannel * channel){
    
    // int i = 0;
    std::ifstream file(filename);
    if (file.is_open())
    {
        // Handle csv header.
        std::string line;
        getline(file, line);
        while (getline(file, line))
        {
            size_t pos = 0;
            pos = line.find(',');
            std::string token = line.substr(0, pos);
            const char * data  = token.c_str();
            (*channel).timePoints.push_back(atof(data));
            line.erase(0, pos + 1);
            data  = line.c_str();
            (*channel).dataPoints.push_back(atof(data));
        }

        // Set the new channel's name to the name of the file.
        int c = 0;
        char * lastSlash = (char *)filename;
        while(*(filename + c) != 0){
            volatile auto temp = filename + c;
            if(filename[c] == '/')
                lastSlash = (char *)filename + c;
            c++;
        }
    
        strcpy((*channel).settingName, lastSlash + 1);
        updateDataChannelFields(*channel);

        file.close();
    }
}