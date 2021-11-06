#include "ControlLayer.h"
#include "ConnectRadar.h"
#include "RadarScanViewLayer.h"

ControlRadar::ControlRadar() :
	_slider(nullptr),
	_scanSpeedText(nullptr),
	_scanFrequence(nullptr),
	_controlButtonLayer(nullptr),
	_maxDistRadioButtonLayer(nullptr),
	_scanModeRadioButtonLayer(nullptr),
	_isConnect(false),
	_isCanChangeMode(true)
{
}

ControlRadar::~ControlRadar()
{
}

bool ControlRadar::init()
{
	if (!Layer::init())return false;

	auto visibleSize = Director::getInstance()->getVisibleSize();
	Vec2 origin = Director::getInstance()->getVisibleOrigin();

	onShowControlLayerButton();

	onShowOpcityBackground();
	onShowConnectText();
	//onShowMaxDistRadioButton();

	return true;
}

void ControlRadar::onCreateData()
{
	checkDeviceHealth();

	_modeVector.clear();
	ConnectRadar::getInstance()->lidar_drv->getAllSupportedScanModes(_modeVector); // 获取支持的扫描模式
	ConnectRadar::getInstance()->lidar_drv->getTypicalScanMode(_usingScanMode);    // 获取默认使用的模式

	MotorCtrlSupport motorSupport;
	ConnectRadar::getInstance()->lidar_drv->checkMotorCtrlSupport(motorSupport);

	_motorInfo.motorCtrlSupport = motorSupport;
	ConnectRadar::getInstance()->lidar_drv->getMotorInfo(_motorInfo);

	//onShowScanModeRadioButton();
	onShowScanFrequence();
}

void ControlRadar::runRadar()
{
	ConnectRadar::getInstance()->lidar_drv->setMotorSpeed();
	ConnectRadar::getInstance()->lidar_drv->startScanExpress(false, _usingScanMode); // 开始扫描

	schedule([&](float delta) { refreshScanData(); }, 1.f / 30.f, "info");  // 定时获取扫描数据

	onShowSlider();
}

void ControlRadar::pauseRadar()
{
	this->unschedule("info");
	ConnectRadar::getInstance()->lidar_drv->stop();
	radarScanViewLayer->setIsScan(false);
}

void ControlRadar::stopRadar()
{
	this->unschedule("info");
	ConnectRadar::getInstance()->lidar_drv->stop();
	radarScanViewLayer->setIsScan(false);
	/* ConnectRadar::getInstance()->lidar_drv->disconnect();
	 ConnectRadar::getInstance()->lidar_drv->reset();*/
}

void ControlRadar::resetRadar()
{
	if (MessageBoxW(nullptr, L"这个设备将会重新启动。", L"你确定吗？", MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
	{
		return;
	}

	pauseRadar();
	ConnectRadar::getInstance()->lidar_drv->reset();
}

void ControlRadar::checkDeviceHealth()
{
	int errorcode;
	if (!ConnectRadar::getInstance()->checkDeviceHealth(&errorcode)) {
		char msg[200];
		sprintf(msg, "设备运转不正常。你需要重新启动设备。\n错误码: 0x%08x", errorcode);

		MessageBox(msg, "Warning");
	}
}

void ControlRadar::refreshScanData()
{
	sl_lidar_response_measurement_node_hq_t nodes[8192];
	size_t cnt = _countof(nodes);
	ILidarDriver* lidar_drv = ConnectRadar::getInstance()->lidar_drv;

	if (IS_OK(lidar_drv->grabScanDataHq(nodes, cnt, 0)))
	{
		float sampleDurationRefresh = _modeVector[_usingScanMode].us_per_sample;
		radarScanViewLayer->setScanData(nodes, cnt, sampleDurationRefresh);
	}
}

void ControlRadar::createButton(Node* node, const Vec2& pos, int bgWith, const std::string& txt, int id)
{
	auto draw = DrawNode::create();
	draw->drawSolidRect(Vec2(0, pos.y - 40), Vec2(bgWith, pos.y + 40), Color4F(0.7f, 0.7f, 0.7f, 0.8f));
	draw->setOpacity(0);
	node->addChild(draw);

	auto text = ui::Text::create();
	text->setString(txt);
	text->setPosition(pos);
	text->setFontSize(30);
	text->setColor(Color3B::WHITE);
	node->addChild(text);
	if (id - 20 == _usingScanMode)
	{
		text->setColor(Color3B(0, 255, 255));
	}

	DNode dn;
	dn.drawNode = draw;
	dn.pos = pos;
	dn.id = id;
	dn.txt = text;
	_drawNode.push_back(dn);
}

void ControlRadar::onShowConnectText()
{
	_connectText = ui::Text::create();
	_connectText->setPosition(Vec2(1580, 1000));
	_connectText->setString("设备未连接！");
	_connectText->setFontSize(20);
	_connectText->setColor(Color3B::RED);
	_connectText->setGlobalZOrder(1);
	this->addChild(_connectText);
}

void ControlRadar::onUpdateConnectText()
{
	_connectText->setString("设备已连接！设备序列号：" + ConnectRadar::getInstance()->getSerialNumber());
	_connectText->setColor(Color3B(0, 255, 255));
}

void ControlRadar::onShowStateText()
{
	_stateText = ui::Text::create();
	_stateText->setPosition(Vec2(1580, 950));
	_stateText->setString("设备状态：未扫描");
	_stateText->setColor(Color3B::RED);
	_stateText->setFontSize(20);
	_stateText->setGlobalZOrder(1);
	this->addChild(_stateText);
}

void ControlRadar::onUpdateStateText(int id)
{
	if (id)
	{
		_stateText->setString("设备状态：正在扫描");
		_stateText->setColor(Color3B(0, 255, 255));
	}
	else
	{
		_stateText->setString("设备状态：未扫描");
		_stateText->setColor(Color3B::RED);
	}
}

void ControlRadar::onShowCurrentSelectDot()
{
	_selectDotText = ui::Text::create();
	_selectDotText->setPosition(Vec2(1580, 900));
	_selectDotText->setFontSize(20);
	_selectDotText->setColor(Color3B(0, 255, 255));
	_selectDotText->setGlobalZOrder(1);
	this->addChild(_selectDotText);
}

void ControlRadar::onUpdateCurrentSelectDot(const float dist, const float angle)
{
	char str[128];
	sprintf(str, "选择障碍物的角度：%.2lf°   距离：%.lfmm / %.2lfm", angle, dist, dist / 1000);
	_selectDotText->setString(str);
}

void ControlRadar::onShowScanSpeed()
{
	_scanSpeedText = ui::Text::create();
	_scanSpeedText->setPosition(Vec2(1580, 850));
	_scanSpeedText->setFontSize(20);
	_scanSpeedText->setColor(Color3B(0, 255, 255));
	_scanSpeedText->setGlobalZOrder(1);
	this->addChild(_scanSpeedText);
}

void ControlRadar::onShowSlider()
{
	if (!_slider)
	{
		auto str = ui::Text::create();
		str->setString("设备转速调整：");
		str->setFontSize(20);
		str->setColor(Color3B(0, 255, 255));
		str->setPosition(Vec2(1480, 800));
		str->setGlobalZOrder(1);
		this->addChild(str);

		_slider = ui::Slider::create();
		_slider->loadBarTexture("Slider_Back.png"); // what the slider looks like
		_slider->loadSlidBallTextures("SliderNode_Normal.png", "SliderNode_Press.png", "SliderNode_Disable.png");
		_slider->loadProgressBarTexture("Slider_PressBar.png");
		_slider->setMaxPercent(_motorInfo.max_speed);
		_slider->setPercent(_motorInfo.desired_speed);
		_slider->setPosition(Vec2(1700, 800));
		_slider->setGlobalZOrder(1);

		_slider->addEventListener([=](Ref* sender, ui::Slider::EventType type) {
			switch (type)
			{
			case cocos2d::ui::Slider::EventType::ON_PERCENTAGE_CHANGED:
				if (_sliderControl)
				{
					ConnectRadar::getInstance()->lidar_drv->setMotorSpeed(_slider->getPercent());
				}
				break;
			}
			});

		this->addChild(_slider);
	}
}

void ControlRadar::onShowScanFrequence()
{
	_scanFrequence = ui::Text::create();
	_scanFrequence->setFontSize(20);
	_scanFrequence->setColor(Color3B(0, 255, 255));
	_scanFrequence->setPosition(Vec2(1580, 750));
	_scanFrequence->setGlobalZOrder(1);
	this->addChild(_scanFrequence);
}

void ControlRadar::onUpdateScanFrequence(int frequence)
{
	if (_scanFrequence)
	{
		_scanFrequence->setString("扫描频率：" + std::to_string(frequence) + " K");
	}
}

void ControlRadar::onShowMaxDistRadioButton()
{
	if (!_maxDistRadioButtonLayer)
	{
		_maxDistRadioButtonLayer = LayerColor::create(Color4B(0, 0, 0, 225));
		_maxDistRadioButtonLayer->setContentSize(Size(200, 400));
		_maxDistRadioButtonLayer->setPosition(Vec2(500, 110));
		this->addChild(_maxDistRadioButtonLayer);

		auto line = DrawNode::create();
		line->drawRect(Vec2(0, 0), Vec2(200, 400), Color4F(0, 1, 1, 1));
		_maxDistRadioButtonLayer->addChild(line, 1);

		createButton(_maxDistRadioButtonLayer, Vec2(100, 360), 200, std::to_string(_showDist[0] / 1000) + "m", 10);
		createButton(_maxDistRadioButtonLayer, Vec2(100, 280), 200, std::to_string(_showDist[1] / 1000) + "m", 11);
		createButton(_maxDistRadioButtonLayer, Vec2(100, 200), 200, std::to_string(_showDist[2] / 1000) + "m", 12);
		createButton(_maxDistRadioButtonLayer, Vec2(100, 120), 200, std::to_string(_showDist[3] / 1000) + "m", 13);
		createButton(_maxDistRadioButtonLayer, Vec2(100, 40), 200, std::to_string(_showDist[4] / 1000) + "m", 14);
	}
	else
	{
		_maxDistRadioButtonLayer->setVisible(true);
	}
}

void ControlRadar::onShowScanModeRadioButton()
{
	if (!_scanModeRadioButtonLayer)
	{
		_scanModeRadioButtonLayer = LayerColor::create(Color4B(0, 0, 0, 225));
		_scanModeRadioButtonLayer->setContentSize(Size(200, 400));
		_scanModeRadioButtonLayer->setPosition(Vec2(500, 30));
		this->addChild(_scanModeRadioButtonLayer);

		auto line = DrawNode::create();
		line->drawRect(Vec2(0, 0), Vec2(200, 400), Color4F(0, 1, 1, 1));
		_scanModeRadioButtonLayer->addChild(line, 1);

		createButton(_scanModeRadioButtonLayer, Vec2(100, 360), 200, _modeVector[0].scan_mode, 20);
		createButton(_scanModeRadioButtonLayer, Vec2(100, 280), 200, _modeVector[1].scan_mode, 21);
		createButton(_scanModeRadioButtonLayer, Vec2(100, 200), 200, _modeVector[2].scan_mode, 22);
		createButton(_scanModeRadioButtonLayer, Vec2(100, 120), 200, _modeVector[3].scan_mode, 23);
		createButton(_scanModeRadioButtonLayer, Vec2(100, 40), 200, _modeVector[4].scan_mode, 24);
	}
	else
	{
		_scanModeRadioButtonLayer->setVisible(true);
	}
}

void ControlRadar::onControlScanFrequence(int id)
{
	for (auto& node : _drawNode)
	{
		if (node.id != id && node.id >= 20 && node.id <= 25)
		{
			node.txt->setColor(Color3B::WHITE);
		}
	}
}

void ControlRadar::onShowControlLayerButton()
{
	auto bt = ui::Button::create();
	bt->setTitleText("≡");
	bt->setTitleFontSize(60);
	bt->setColor(Color3B::WHITE);
	bt->setGlobalZOrder(1);
	bt->setPosition(Vec2(50, 1030));
	this->addChild(bt);

	bt->addTouchEventListener([=](Ref* sender, ui::Widget::TouchEventType type)
		{
			switch (type)
			{
			case cocos2d::ui::Widget::TouchEventType::ENDED:
				onShowControlButtonLayer();
				break;
			}
		});
}

void ControlRadar::onShowControlButtonLayer()
{
	static int i = 0;
	if (!_controlButtonLayer)
	{
		_controlButtonLayer = LayerColor::create(Color4B(0, 0, 0, 225));
		_controlButtonLayer->setContentSize(Size(500, 1080));
		_controlButtonLayer->setPosition(Vec2(-500, 0));
		this->addChild(_controlButtonLayer);

		auto line = DrawNode::create();
		line->drawRect(Vec2(-1, 1), Vec2(499, 1078), Color4F(0, 1, 1, 1));
		_controlButtonLayer->addChild(line, 1);

		onShowControlButton();
	}

	_controlButtonLayer->runAction(MoveBy::create(0.2f, Vec2(++i % 2 ? 500 : -500, 0)));
}

void ControlRadar::onUpdateControlButtonBackGround(const Vec2& pos)
{
	for (auto& node : _drawNode)
	{
		if (node.id >= 0 && node.id <= 8)
		{
			if (pos.x > 0 && pos.x<500 && pos.y>node.pos.y - 40 && pos.y < node.pos.y + 40)
			{
				if (node.id == 0)
				{
					if (!_isConnect)
					{
						node.drawNode->setOpacity(200);
					}
				}
				else if (node.id >= 1 && node.id < 5)
				{
					if (_isConnect)
					{
						node.drawNode->setOpacity(200);
					}
				}
				else if (node.id >= 5 && node.id <= 6)
				{
					node.drawNode->setOpacity(200);
				}
				else if (node.id == 7)
				{
					if (_isConnect && _isCanChangeMode)
					{
						node.drawNode->setOpacity(200);
					}
				}
				else if (node.id == 8)
				{
					node.drawNode->setOpacity(200);
				}
				else
				{
					node.drawNode->setOpacity(0);
				}
			}
			else
			{
				node.drawNode->setOpacity(0);
			}
		}
		else if (node.id >= 10 && node.id <= 15)
		{
			if (pos.x > 500 && pos.x<700 && pos.y > node.pos.y - 40 + 100 && pos.y < node.pos.y + 40 + 100)
			{
				node.drawNode->setOpacity(200);
			}
			else
			{
				node.drawNode->setOpacity(0);
			}
		}
		else if (node.id >= 20 && node.id <= 25)
		{
			if (pos.x > 500 && pos.x<700 && pos.y > node.pos.y - 40 + 30 && pos.y < node.pos.y + 40 + 30)
			{
				node.drawNode->setOpacity(200);
			}
			else
			{
				node.drawNode->setOpacity(0);
			}
		}
	}
}

void ControlRadar::onButtonEventListener(const Vec2& pos)
{
	for (auto& node : _drawNode)
	{
		if (pos.x > 0 && pos.x<500 && pos.y>node.pos.y - 40 && pos.y < node.pos.y + 40)
		{
			if (node.id == 0)
			{
				if (!_isConnect)
				{
					if (ConnectRadar::getInstance()->onConnect())
					{
						onUpdateConnectText();
						onShowStateText();
						onShowCurrentSelectDot();
						onShowScanSpeed();
						onCreateData();

						_isConnect = true;
					}
				}
			}
			else if (node.id == 1)
			{
				if (_isConnect)
				{
					stopRadar();
					onUpdateStateText(0);
					if(_slider)_slider->setEnabled(false);
					_isCanChangeMode = false;
				}
			}
			else if (node.id == 2)
			{
				if (_isConnect)
				{
					runRadar();
					onUpdateStateText(1);
					if (_slider)_slider->setEnabled(true);
					_isCanChangeMode = false;
				}
			}
			else if (node.id == 3)
			{
				if (_isConnect)
				{
					pauseRadar();
					onUpdateStateText(0);
					if (_slider)_slider->setEnabled(false);
					_isCanChangeMode = true;
				}
			}
			else if (node.id == 4)
			{
				if (_isConnect)
				{
					resetRadar();
					_isCanChangeMode = true;
				}
			}
			else if (node.id == 5)
			{
				radarScanViewLayer->setMoveLayerPosition(Vec2::ZERO);
			}
			else if (node.id == 6)
			{
				onShowMaxDistRadioButton();
				break;
			}
			else if (node.id == 7)
			{
				if (_isConnect && _isCanChangeMode)
				{
					onShowScanModeRadioButton();
					break;
				}
			}
			else if (node.id == 8)
			{
				radarScanViewLayer->setShowLaserLine();
			}
		}
		else if (pos.x > 500 && pos.x<700 && pos.y>node.pos.y - 40 + 100 && pos.y < node.pos.y + 40 + 100)
		{
			if (node.id >= 10 && node.id <= 15)
			{
				radarScanViewLayer->setCurrentDisplayRange(_showDist[node.id - 10]);
				_maxDistRadioButtonLayer->setVisible(false);
			}
		}
		else if (pos.x > 500 && pos.x<700 && pos.y>node.pos.y - 40 + 30 && pos.y < node.pos.y + 40 + 30)
		{
			if (node.id >= 20 && node.id <= 25)
			{
				_usingScanMode = node.id - 20;
				node.txt->setColor(Color3B(0, 255, 255));
				_scanModeRadioButtonLayer->setVisible(false);
				onControlScanFrequence(node.id);
			}
		}
		else
		{
			if (_maxDistRadioButtonLayer)_maxDistRadioButtonLayer->setVisible(false);
			if (_scanModeRadioButtonLayer)_scanModeRadioButtonLayer->setVisible(false);
		}
	}
}

void ControlRadar::onShowControlButton()
{
	std::string name[] = { {"连接"},{"关闭"},{"开始"},{"暂停"},{"重新启动"},{"图像复位"},{"显示距离"},{"扫描模式"},{"显示激光束"}};
	createButton(_controlButtonLayer, Vec2(250, 950), 500, name[0], 0); // 连接
	createButton(_controlButtonLayer, Vec2(250, 870), 500, name[1], 1); // 关闭
	createButton(_controlButtonLayer, Vec2(250, 790), 500, name[2], 2); // 开始
	createButton(_controlButtonLayer, Vec2(250, 710), 500, name[3], 3); // 暂停
	createButton(_controlButtonLayer, Vec2(250, 630), 500, name[4], 4); // 从新启动
	createButton(_controlButtonLayer, Vec2(250, 550), 500, name[5], 5); // 图像复位
	createButton(_controlButtonLayer, Vec2(250, 470), 500, name[6], 6); // 显示距离
	createButton(_controlButtonLayer, Vec2(250, 390), 500, name[7], 7); // 扫描模式
	createButton(_controlButtonLayer, Vec2(250, 310), 500, name[8], 8); // 显示激光束
}

void ControlRadar::onUpdateScanSpeed()
{
	if (_scanSpeedText)
	{
		char str[128];
		sprintf(str, "当前设备转速：%.1lf HZ  /  %d rpm", radarScanViewLayer->getScanSpeed(), (int)(radarScanViewLayer->getScanSpeed() * 60));
		_scanSpeedText->setString(str);
	}
}

void ControlRadar::onShowOpcityBackground()
{
	auto opl = LayerColor::create(Color4B(0, 0, 0, 200));
	opl->setContentSize(Size(660, 320));
	opl->setPosition(Vec2(1250, 730));
	opl->setGlobalZOrder(1);
	this->addChild(opl);

	auto draw = DrawNode::create();
	draw->drawRect(Vec2(1250, 730), Vec2(1910, 1050), Color4F(0, 1, 1, 1));
	draw->setGlobalZOrder(1);
	draw->setLineWidth(1);
	this->addChild(draw);
}