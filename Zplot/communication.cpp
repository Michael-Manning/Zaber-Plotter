#include <zaber/za_serial.h>
#include <iostream>
#include <string>
#include <cstring>
#include <assert.h>
#include <stdio.h>
#include <chrono>
#include <thread>
#include <mutex>

#include "communication.h"

using namespace std;


namespace{
z_port port;
bool deviceConnected = false;
mutex sendLock;
mutex sendLockAsync;
}

namespace communication {

	bool isConnected(){
		return deviceConnected;
	}

	bool connect(const char * portName) {
		if(deviceConnected)
			disconnect();
		if (za_connect(&port, portName) != Z_SUCCESS)
		{
			cout << "Could not connect to device %s.\n" << endl;
			deviceConnected = false;
			return false;
		}
		deviceConnected = true;
		return true;
	}

	void disconnect(){
		if(deviceConnected){
			za_disconnect(port);
		}
		deviceConnected = false;
	}


	// void sendThreadFunc(const char * command){
	// 	assert(strlen(command) > 0);
	// 	sendAndWait(command);
	// }

	// thread sendThread;
	// void sendAsync(const char * command) {
	// 	// Only would be necessary if this function itself were called from multiple threads
	// 	std::lock_guard<std::mutex> lock(sendLockAsync);
	// 	if(sendThread.joinable()){
	// 		sendThread.detach();
	// 	}
	// 	assert(strlen(command) > 0);
	// 	sendThread = thread(sendThreadFunc, command);
	// }

	bool receive(char reply[256], float * data) {
		std::lock_guard<std::mutex> lock(sendLock);
		int er = za_receive(port, reply, 256);

		// Most likely a timeout if error.
		if (er < 0)
			return false;

		if (DebugMode)
			cout << "debug << " << reply << endl;
		receiveCallback(reply);
		
		za_reply decodedReply;
		za_decode(&decodedReply, reply);
		if (strcmp(decodedReply.reply_flags, "OK"))
			return false;

		if(data != NULL)
			*data = std::stof(decodedReply.response_data);
		return true;
	}

	bool sendAndWait(const char * command, float * data) {
		// This mutex slows things down a lot. It was replaced by
		// A thread safe que which is used by the live poll thread in main.cpp
		//std::lock_guard<std::mutex> lock(sendLock);
		
		char reply[256] = { 0 };
		za_reply decoded_reply;

		assert(strlen(command) > 0);
		
		if (DebugMode)
			cout << "debug >> " << command;
		receiveCallback((char*)command);

		za_send(port, command);
		int er = za_receive(port, reply, sizeof(reply));

		// Most likely a timeout if error.
		 if(er < 0)
			return false;

		if (DebugMode)
			cout << "debug << " << reply << endl;
		receiveCallback(reply);

		za_reply decodedReply;
		za_decode(&decodedReply, reply);
		if (strcmp(decodedReply.reply_flags, "OK"))
			return false;

		if (data != NULL)
			*data = stof(decodedReply.response_data);
		return true;
	}

	void pollUntilIdle(){
		if(!isConnected())
			return;

		char reply[256] = { 0 };
		struct za_reply decodedReply;

		while(true)
		{
			za_send(port, "/\n");
			cout << "debug >> /\\n" << endl;
			int error = za_receive(port, reply, sizeof(reply));
			cout << "debug << " << reply << endl;
			za_decode(&decodedReply, reply);

			if(
				error < 0 ||
				strcmp(decodedReply.reply_flags, "OK") != 0 ||
				strncmp(decodedReply.device_status, "BUSY", 4) == 0)
			{
				this_thread::sleep_for(std::chrono::milliseconds(50));
			}
			else 
			{
				break;
			}
		}
		// Just for good measure.
		sendAndWait("/\n"); 

		while(receive(reply));
	}
}
