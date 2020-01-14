//This file is no longer used.

#if 0
#pragma once
#include <simpleIni/SimpleIni.h>

namespace configuration {

	enum class SettingType {
		Boolean,
		Integer,
		Float,
		String,
		Letter
	};

	struct setting {
		//const char * name;
		std::string name;
		SettingType type;

		union {
			bool b;
			int i;
			float f;
			const char * s;
			char c;
		};

		operator bool const() {
			if (type == SettingType::Boolean)
				return b;
			return false;
		}
		operator float() {
			if (type == SettingType::Float)
				return f;
			if (type == SettingType::Integer)
				return i;
			return 0;
		}
		operator int() {
			if (type == SettingType::Integer)
				return i;
			return 0;
		}
	};

	// Handles loading initial settins and creating a config file if there isn't one.
	void initConfig();
	
	// Retrieves a setting given it's name
	setting getSetting(const char * settingName);
	
	// Loads default settings into memory or resets them if they are already loaded.
	void loadDefaults();
}

#endif