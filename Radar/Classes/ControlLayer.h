#pragma once

#include "rplidar.h"
#include "cocos2d.h"
#include "ui/CocosGUI.h"
#include "Define.h"

using namespace sl;
using namespace cocos2d;

class RadarScanViewLayer;

class ControlRadar :public cocos2d::Layer
{
public:
	CREATE_FUNC(ControlRadar);

	ControlRadar();
	~ControlRadar();

	virtual bool init();

	void onCreateData();
	void runRadar();
	void pauseRadar();
	void stopRadar();
	void resetRadar();
	void onUpdateCurrentSelectDot(const float dist, const float angle);
	void onUpdateScanSpeed();
	void onUpdateScanFrequence(int frequence);
	void onUpdateControlButtonBackGround(const Vec2& pos);
	void onButtonEventListener(const Vec2& pos);

protected:
	void checkDeviceHealth();
	void refreshScanData();
	void createButton(Node* node, const Vec2& pos, int bgWith, const std::string& txt, int id);
	void onShowConnectText();
	void onUpdateConnectText();
	void onShowStateText();
	void onUpdateStateText(int id);
	void onShowCurrentSelectDot();
	void onShowOpcityBackground();
	void onShowScanSpeed();
	void onShowSlider();
	void onShowScanFrequence();
	void onShowMaxDistRadioButton();
	void onShowScanModeRadioButton();
	void onControlScanFrequence(int id);
	void onShowControlLayerButton();
	void onShowControlButton();
	void onShowControlButtonLayer();

protected:
	typedef struct 
	{
		DrawNode* drawNode;
		Vec2 pos;
		int id;
		ui::Text* txt;
	} DNode;
	sl_u16 _usingScanMode; //扫描模式
	std::vector<LidarScanMode> _modeVector;
	std::vector<DNode> _drawNode;
	ui::Text* _connectText;
	ui::Text* _stateText;
	ui::Text* _selectDotText;
	ui::Text* _scanSpeedText;
	ui::Text* _scanFrequence;
	ui::Slider* _slider;
	std::string _serialNumber;
	LidarMotorInfo _motorInfo;
	LayerColor* _controlButtonLayer;
	LayerColor* _maxDistRadioButtonLayer;
	LayerColor* _scanModeRadioButtonLayer;
	bool _sliderControl;
	bool _isConnect;
	bool _isCanChangeMode;
	int _showDist[5] = { 6000,8000,10000,16000,35000 };
};