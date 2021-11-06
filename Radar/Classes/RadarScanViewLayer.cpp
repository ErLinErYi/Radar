#include "RadarScanViewLayer.h"
#include "Define.h"
#include "ControlLayer.h"

#define RADAR_DEFAULT_DIST 8000
#define OUT_VIEW -Vec2(10000,10000)

RadarScanViewLayer::RadarScanViewLayer() :
	_isScanning(false),
	_isShowLaserLine(false),
	_scanSpeed(0.f),
	_currentDisPlayRange(RADAR_DEFAULT_DIST),
	_drawScan(0),
	_scrollEnlarge(0),
	_mouseAngle(0.f),
	_mousePoint(Vec2::ZERO),
	_direction{ "前","右","后","左" },
	_windowCenter(Vec2(600, Director::getInstance()->getWinSize().height / 2.f)),
	_drawNode(DrawNode::create())
{
}

RadarScanViewLayer::~RadarScanViewLayer()
{
}

bool RadarScanViewLayer::init()
{
	if (!LayerColor::initWithColor(Color4B(70, 70, 70, 100)))return false;

	_moveLayer = LayerColor::create();
	this->addChild(_moveLayer);

	drawBackGround();
	onMouseListener();

	schedule([&](float delta) { drawScanData(); }, 1.f / 30.f, "drawinfo");  // 定时画出数据

	return true;
}

void RadarScanViewLayer::setScanData(sl_lidar_response_measurement_node_hq_t* buffer, size_t count, float sampleDuration)
{
	_scanData.clear();
	_isScanning = true;
	for (int pos = 0; pos < (int)count; ++pos) {
		scanData dot;
		if (!buffer[pos].dist_mm_q2) continue;

		dot.quality = buffer[pos].quality;
		dot.angle = buffer[pos].angle_z_q14 * 90.f / 16384.f;
		dot.dist = buffer[pos].dist_mm_q2 / 4.0f;
		_scanData.push_back(dot);
	}

	_sampleDuration = sampleDuration;
	_scanSpeed = 1000000.0f / (count * sampleDuration);
}

float RadarScanViewLayer::getScanSpeed()
{
	return _scanSpeed;
}

void RadarScanViewLayer::setIsScan(bool isScan)
{
	if (!isScan)
	{
		_scanSpeed = 0;
		_isScanning = false;
	}
}

void RadarScanViewLayer::setCurrentDisplayRange(int range)
{
	_currentDisPlayRange = range;

	onDrawingGraphics();
}

void RadarScanViewLayer::setMoveLayerPosition(const Vec2& pos)
{
	_moveLayer->setPosition(pos);
	_scrollEnlarge = 0;

	onDrawingGraphics();
}

void RadarScanViewLayer::setShowLaserLine()
{
	_isShowLaserLine = !_isShowLaserLine;
}

void RadarScanViewLayer::drawScanData()
{
	_drawNode->clear();

	_drawScan < 360 ? _drawScan += 5 : _drawScan = 0;
	for (float i = 0; i < 36; i += 0.05f)
	{
		auto dist = _scrollEnlarge > -50 ? _scrollEnlarge : -50;
		auto rad = (i + _drawScan) * M_PI / 180.f;
		auto posX = sin(rad) * (500 + dist) + _windowCenter.x;
		auto posY = cos(rad) * (500 + dist) + _windowCenter.y;

		auto opc = i / 360.f;
		opc < 0.1 ? opc /= 2.f : opc = 0;
		_drawNode->drawLine(_windowCenter, Vec2(posX, posY), Color4F(0.f, 1.f, 1.f, opc));
	}

	const auto range = std::min(_windowCenter.x, _windowCenter.y) - 40 + _scrollEnlarge;
	const auto distScale = range / _currentDisPlayRange;

	float minDangle = 50;
	int selectPoint = 0;
	int i = 0;
	for (auto& dot : _scanData)
	{
		auto dist = dot.dist * distScale;
		auto rad = dot.angle * M_PI / 180.f;
		auto posX = sin(rad) * dist + _windowCenter.x;
		auto posY = cos(rad) * dist + _windowCenter.y;
		auto dangle = fabs(rad - _mouseAngle);
		//CCLOG("%d", dot.quality);
		if (dangle < minDangle)
		{
			minDangle = dangle;
			selectPoint = i;
		}

		//_drawNode->drawCircle(pos, 1.f, M_PI, 20, false, Color4F::GREEN);
		if (_isShowLaserLine)
		{
			_drawNode->drawDot(Vec2(posX, posY), 4.f, Color4F::GREEN);
			_drawNode->drawLine(_windowCenter, Vec2(posX, posY), Color4F::YELLOW);
		}
		else
		{
			_drawNode->drawDot(Vec2(posX, posY), 2.f, Color4F::GREEN);
		}

		++i;
	}

	onMouseSelectPoint(selectPoint, distScale);

	controlRadar->onUpdateScanSpeed();
	controlRadar->onUpdateScanFrequence(_isScanning ? int(floor(1000.0 / _sampleDuration + 0.5)) : 0);
}

void RadarScanViewLayer::drawBackGround()
{
	_bgDrawNode = DrawNode::create();
	_bgDrawNode->setLineWidth(1.f);
	_moveLayer->addChild(_bgDrawNode);
	_moveLayer->addChild(_drawNode);

	onDrawingGraphics();
}

void RadarScanViewLayer::drawDistText(const int distRefresh, const int drawCheckNumber, const float scale)
{
	if (!_refreshDistStr.count(distRefresh))
	{
		auto strDist = ui::Text::create();
		strDist->setString(std::to_string(static_cast<int>(round(scale))) + " (mm)");
		strDist->setPosition(_windowCenter + Vec2(0, drawCheckNumber + 30));
		strDist->setFontSize(25);
		_moveLayer->addChild(strDist);

		_refreshDistStr.insert(std::pair<int, ui::Text*>(distRefresh, strDist));
	}
	else
	{
		auto p = _refreshDistStr.find(distRefresh);
		if (p != _refreshDistStr.end())
		{
			p->second->setPosition(_windowCenter + Vec2(0, drawCheckNumber + 30));
			p->second->setString(std::to_string(static_cast<int>(round(scale))) + " (mm)");
		}
	}
}

void RadarScanViewLayer::drawAngleText(const Vec2& pos, const int angleRefresh, const int i)
{
	if (!_refreshAngleStr.count(angleRefresh))
	{
		auto number = ui::Text::create();
		number->setString(std::to_string(i) + "°");
		number->setRotation(i);
		number->setFontSize(18);
		number->setPosition(pos);
		_moveLayer->addChild(number);

		_refreshAngleStr.insert(std::pair<int, ui::Text*>(angleRefresh, number));
	}
	else
	{
		auto p = _refreshAngleStr.find(angleRefresh);
		if (p != _refreshAngleStr.end())
		{
			p->second->setPosition(pos);
		}
	}
}

void RadarScanViewLayer::drawTextText(const Vec2& pos, const int textRefresh, const int i)
{
	if (!_refreshTextStr.count(textRefresh))
	{
		auto str = ui::Text::create();
		str->setString(_direction[i / 90]);
		str->setPosition(pos);
		str->setFontSize(30);
		_moveLayer->addChild(str);

		_refreshTextStr.insert(std::pair<int, ui::Text*>(textRefresh, str));
	}
	else
	{
		auto p = _refreshTextStr.find(textRefresh);
		if (p != _refreshTextStr.end())
		{
			p->second->setPosition(pos);
		}
	}
}

void RadarScanViewLayer::onDrawingGraphics()
{
	_bgDrawNode->clear();

	int drawCheckNumber = 500 + _scrollEnlarge;
	int distRefresh = -1;
	int angleRefresh = -1;
	int textRefresh = -1;

	if (_scrollEnlarge < -50) 
	{
		drawCheckNumber = 450;
	}

	do
	{
		if (drawCheckNumber < 1400)
		{
			_bgDrawNode->drawCircle(_windowCenter, drawCheckNumber, M_PI, 100, false, Color4F(0, 1, 1, 1));

			++distRefresh;
			drawDistText(distRefresh, drawCheckNumber, drawCheckNumber * _currentDisPlayRange / (500 + _scrollEnlarge));

			if (drawCheckNumber > 300)
			{
				for (int i = 0; i < 360; ++i)
				{
					auto rad = i * M_PI / 180.f;

					auto posB_D_X = sin(rad) * drawCheckNumber + _windowCenter.x;
					auto posB_D_Y = cos(rad) * drawCheckNumber + _windowCenter.y;

					if (i % 10)
					{
						auto posB_O_X = sin(rad) * (drawCheckNumber - 20) + _windowCenter.x;
						auto posB_O_Y = cos(rad) * (drawCheckNumber - 20) + _windowCenter.y;

						_bgDrawNode->drawLine(Vec2(posB_O_X, posB_O_Y), Vec2(posB_D_X, posB_D_Y), Color4F(0.f, 1.f, 1.f, 0.3f));
					}
					else
					{
						auto posB_D_T_X = sin(rad) * (drawCheckNumber + 10) + _windowCenter.x;
						auto posB_D_T_Y = cos(rad) * (drawCheckNumber + 10) + _windowCenter.y;

						_bgDrawNode->drawLine(_windowCenter, Vec2(posB_D_X, posB_D_Y), Color4F(0.f, 1.f, 1.f, 0.5f));

						++angleRefresh;
						drawAngleText(Vec2(posB_D_T_X, posB_D_T_Y), angleRefresh, i);
					}

					if (i % 90 == 0)
					{
						auto posB_D_CT_X = sin(rad) * (drawCheckNumber + 60) + _windowCenter.x;
						auto posB_D_CT_Y = cos(rad) * (drawCheckNumber + 60) + _windowCenter.y;

						++textRefresh;
						drawTextText(Vec2(posB_D_CT_X, posB_D_CT_Y), textRefresh, i);
					}
				}
			}
		}
		drawCheckNumber -= 200;

	} while (drawCheckNumber >= 100);

	for (int i = distRefresh + 1; i <= _refreshDistStr.size(); ++i)
	{
		auto p = _refreshDistStr.find(i);
		if (p != _refreshDistStr.end())
		{
			p->second->setPosition(OUT_VIEW);
		}
	}

	for (int i = angleRefresh + 1; i <= _refreshAngleStr.size(); ++i)
	{
		auto p = _refreshAngleStr.find(i);
		if (p != _refreshAngleStr.end())
		{
			p->second->setPosition(OUT_VIEW);
		}
	}

	for (int i = textRefresh + 1; i < _refreshTextStr.size(); ++i)
	{
		auto p = _refreshTextStr.find(i);
		if (p != _refreshTextStr.end())
		{
			p->second->setPosition(OUT_VIEW);
		}
	}
}

void RadarScanViewLayer::onMouseSelectPoint(const int i, const float scale)
{
	if (_scanData.size() > i)
	{
		auto dist = _scanData[i].dist * scale;
		auto rad = _scanData[i].angle * M_PI / 180.f;
		auto posX = sin(rad) * dist + _windowCenter.x;
		auto posY = cos(rad) * dist + _windowCenter.y;

		_drawNode->drawDot(Vec2(posX, posY), 4.f, Color4F::RED);
		_drawNode->drawLine(_windowCenter, Vec2(posX, posY), Color4F::RED);
		//CCLOG("距离：%lf 角度：%lf", _scanData[i].dist, _scanData[i].angle);

		controlRadar->onUpdateCurrentSelectDot(_scanData[i].dist, _scanData[i].angle);
	}
}

void RadarScanViewLayer::onMouseListener()
{
	static Vec2 phasePosition = Vec2(Vec2::ZERO);
	auto mouseListener = EventListenerMouse::create();

	mouseListener->onMouseScroll = [&](Event* event)
	{
		auto sy = ((EventMouse*)event)->getScrollY() * 20;

		if (_scrollEnlarge > -400 || sy >= 0) // 当缩小倍数小于一定值或有放大趋势时可以改变
		{
			_scrollEnlarge += sy;
		}

		onDrawingGraphics();
	};

	mouseListener->onMouseMove = [&](Event* event)
	{
		_mousePoint = ((EventMouse*)event)->getLocationInView();

		auto dx = _mousePoint.x - _windowCenter.x - _moveLayer->getPosition().x;
		auto dy = (_mousePoint.y - _windowCenter.y) - _moveLayer->getPosition().y;

		if (dx >= 0)
		{
			_mouseAngle = atan2(dx, dy);
		}
		else
		{
			_mouseAngle = M_PI * 2 - atan2(-dx, dy);
		}

		controlRadar->onUpdateControlButtonBackGround(_mousePoint);
	};

	mouseListener->onMouseDown = [&](Event* event)
	{
		_mousePoint = ((EventMouse*)event)->getLocationInView();

		controlRadar->onButtonEventListener(_mousePoint);
	};

	auto touchListener = EventListenerTouchOneByOne::create();

	touchListener->onTouchBegan = [=](Touch* t, Event* e) {
		phasePosition = t->getLocation() - _moveLayer->getPosition();
		return true;
	};
	touchListener->onTouchMoved = [=](Touch* t, Event* e) {
		_mousePoint = t->getLocation();

		_moveLayer->setPosition(_mousePoint - phasePosition);
	};

	_director->getEventDispatcher()->addEventListenerWithSceneGraphPriority(mouseListener, this);
	_director->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, this);
}