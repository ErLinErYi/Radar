#pragma once

#include <string>

#include "rplidar.h"

using namespace sl;

class ConnectRadar
{
public:
	ConnectRadar();
	~ConnectRadar();

	static ConnectRadar* getInstance();

	bool onConnect(std::string port = "\\\\.\\com3", int baudrate = 115200);
	bool isConnected();
	bool checkDeviceHealth(int* errorcode = nullptr);
	void onDisconnect();
	std::string getSerialNumber();

public:
	static ILidarDriver* lidar_drv;

protected:
	static ConnectRadar* instance;
	sl_lidar_response_device_info_t devinfo;
	IChannel* _channel;
	bool _isConnected;
};