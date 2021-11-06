#include "ConnectRadar.h"

ConnectRadar* ConnectRadar::instance = nullptr;
ILidarDriver* ConnectRadar::lidar_drv = nullptr;

ConnectRadar::ConnectRadar():
	_isConnected(false)
{
}

ConnectRadar::~ConnectRadar()
{
	onDisconnect();

	delete instance;
	instance = nullptr;
}

ConnectRadar* ConnectRadar::getInstance()
{
	if (instance == nullptr)
	{
		instance = new ConnectRadar();
	}

	return instance;
}

bool ConnectRadar::onConnect(std::string port, int baudrate)
{
	if (_isConnected)return true;

	_channel = (*createSerialPortChannel(port, baudrate));

	if (!lidar_drv)
	{
		lidar_drv = *createLidarDriver();

		if (!lidar_drv)
		{
			return SL_RESULT_OPERATION_FAIL;
		}
	}

	if (SL_IS_FAIL(lidar_drv->connect(_channel)) || SL_IS_FAIL(lidar_drv->getDeviceInfo(devinfo)))
	{
		return _isConnected;
	}

	_isConnected = true;
	if (!checkDeviceHealth())
	{
		_isConnected = false;
	}

	return _isConnected;
}

void ConnectRadar::onDisconnect()
{
	if (_isConnected)
	{
		lidar_drv->stop();
	}

	_isConnected = false;

	delete lidar_drv;
	lidar_drv = nullptr;
}

std::string ConnectRadar::getSerialNumber()
{
	std::string serialNumber;
	char titleMsg[200];
	for (int pos = 0, startpos = 0; pos < sizeof(devinfo.serialnum); ++pos)
	{
		sprintf(&titleMsg[startpos], "%02X", devinfo.serialnum[pos]);
		startpos += 2;
	}
	serialNumber = titleMsg;

	return serialNumber;
}

bool ConnectRadar::isConnected()
{
	return _isConnected;
}

bool ConnectRadar::checkDeviceHealth(int* errorcode)
{
	int errcode = 0;
	bool ans = false;

	do {
		if (!_isConnected) {
			errcode = SL_RESULT_OPERATION_FAIL;
			break;
		}

		sl_lidar_response_device_health_t healthinfo;
		if (IS_FAIL(lidar_drv->getHealth(healthinfo))) {
			errcode = SL_RESULT_OPERATION_FAIL;
			break;
		}

		if (healthinfo.status != SL_LIDAR_STATUS_OK) {
			errcode = healthinfo.error_code;
			break;
		}

		ans = true;
	} while (0);

	/* 如果出现问题 */
	if (!ans)
	{
		// do
		*errorcode = errcode;
	}

	return ans;
}
