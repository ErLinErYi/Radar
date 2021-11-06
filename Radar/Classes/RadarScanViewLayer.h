#pragma once
#include "cocos2d.h"
#include "rplidar.h"
#include "ui/CocosGUI.h"

using namespace cocos2d;
using namespace sl;

class RadarScanViewLayer:public LayerColor
{
public:
	CREATE_FUNC(RadarScanViewLayer);

	RadarScanViewLayer();
	~RadarScanViewLayer();

	virtual bool init() override;
	void setScanData(sl_lidar_response_measurement_node_hq_t* buffer, size_t count, float sampleDuration);
	float getScanSpeed();
	void setIsScan(bool isScan);
	void setCurrentDisplayRange(int range);
	void setMoveLayerPosition(const Vec2& pos);
	void setShowLaserLine();

protected:
	virtual void drawScanData();
	virtual void drawBackGround();
	virtual void drawDistText(const int distRefresh, const int drawCheckNumber, const float scale);
	virtual void drawAngleText(const Vec2& pos, const int angleRefresh, const int i);
	virtual void drawTextText(const Vec2& pos, const int textRefresh, const int i);
	virtual void onMouseListener();
	virtual void onDrawingGraphics();
	virtual void onMouseSelectPoint(const int i, const float scale);

private:
	struct scanData {
		sl_u8 quality;
		float angle;
		float dist;
	};
	std::vector<scanData> _scanData;

	bool _isScanning;                            // 是否开始扫描
	bool _isShowLaserLine;                       // 是否显示激光线
	float _sampleDuration;                       // 扫描时间
	float _scanSpeed;                            // 扫描速度
	DrawNode* _drawNode;                         // 绘制结点
	DrawNode* _bgDrawNode;                       // 背景绘制结点
	Vec2 _windowCenter;                          // 窗口中心点
	Vec2 _mousePoint;                            // 鼠标坐标
	float _mouseAngle;                           // 鼠标角度
	float _currentDisPlayRange;                  // 当前显示的范围
	float _drawScan;                             // 扫描动图数值
	float _scrollEnlarge;                        // 鼠标放大数值
	std::string  _direction[4];                  // 方位名称
	std::map<int, ui::Text*> _refreshDistStr;    // 距离刷新数据
	std::map<int, ui::Text*> _refreshAngleStr;   // 角度刷新数据
	std::map<int, ui::Text*> _refreshTextStr;    // 文本刷新数据
	LayerColor* _moveLayer;
};
