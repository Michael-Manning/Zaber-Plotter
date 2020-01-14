#pragma once


/*
* All communication related functions are being kept here for now.
* When the C++ Zaber motion library is released it should replace all of this
* as it will better support changes related to the ascii protocol.
*
* As of now, communication relies on the C API to make serial communication easier,
* though there are a few issues such as using port numbers larger than 9 on windows.
*/

	extern void (*receiveCallback)(char reply[256]);
namespace communication {


	const bool DebugMode = false;

	bool isConnected();

	//connects to a device
	bool connect(const char * portName);

	//gracefully disconnect from the connected device
	void disconnect();

	//sends a command
	//void sendAsync(const char * command);

	//recives data until the next newline. Optional field for retrieved data. Returns true for "OK" and false for "RJ" or other errors.
	bool receive(char reply[256], float * data = NULL);

	//sends a command and blocks until a reply is recieved. Returns true for "OK" and false for "RJ" or other errors. Optional field for retrieved data
	bool sendAndWait(const char * command, float * data = NULL);

	//call this emediately after scope start to poll until idle and retrieve the scope data. Returns succsess.
	//bool retrieveScopeData(std::vector<vector<float>> &data);

	void pollUntilIdle();
}

