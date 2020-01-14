//This file is no longer used.


#if 0

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#include <simpleIni/SimpleIni.h>
#include "configuration.h"
#include "default_settings.h"

using namespace std;

CSimpleIniA ini;



namespace configuration {

	const char *  configFileName = "configuration.ini";
	const char *  defaultSettingsFileName = "default_settings.csv";

	bool initialized = false;;

	//map<const char *, setting, cmp_str> settingHash;
	map<string, setting> settingHash;
	setting getSetting(const char * settingName) {
		return settingHash[settingName];
	}


	std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str)
	{
		std::vector<std::string>   result;
		std::string                line;
		std::getline(str, line);

		std::stringstream          lineStream(line);
		std::string                cell;

		while (std::getline(lineStream, cell, ','))
		{
			result.push_back(cell);
		}
		// This checks for a trailing comma with no data after it.
		if (!lineStream && cell.empty())
		{
			// If there was a trailing comma then add an empty element.
			result.push_back("");
		}
		return result;
	}


	//create a script to generate a defaults header without the csv
	void loadDefaults()
	{
		constexpr int settingCount = sizeof(defaultSettings) / sizeof(setting);

		for (int i = 0; i < settingCount; i++)
		{
			settingHash[defaultSettings[i].name] = defaultSettings[i];
		}
	}

	void initConfig()
	{
		loadDefaults();
		ini.SetUnicode();
		SI_Error er = ini.LoadFile(configFileName);


		// Check if no file exsists and load defaults if so.
		if (er == SI_Error::SI_FILE) {
			
			ini.SetBoolValue("testSection", "key", true);
			ini.SaveFile(configFileName, false);
			//{
			//	std::ofstream outfile();
			//	outfile.open(rgszTestFile[1], std::ofstream::out | std::ofstream::binary);
			//	if (ini.Save(outfile, true) < 0) throw false;
			//	outfile.close();
			//}
		//	CurrentConfiguration::loadDefaults();
		//	CurrentConfiguration::darkTheme;
		}

		ini.SetBoolValue("testSection", "key", false);
		ini.SaveFile(configFileName, false);


		/*const char* pVal = ini.GetValue("section", "key", "default");
		ini.SetValue("section", "key", "newvalue");
*/
		initialized = true;
	}

}

#endif