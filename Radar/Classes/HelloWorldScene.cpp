/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/


#include <signal.h>
#include <string.h>
#include <conio.h>

#include "HelloWorldScene.h"
#include "ui/CocosGUI.h"
#include "ConnectRadar.h"
#include "ControlLayer.h"
#include "RadarScanViewLayer.h"

#ifdef _WIN32
#include <Windows.h>
#define delay(x)   ::Sleep(x)
#else
#include <unistd.h>
static inline void delay(sl_word_size_t ms) {
    while (ms >= 1000) {
        usleep(1000 * 1000);
        ms -= 1000;
    };
    if (ms != 0)
        usleep(ms * 1000);
}
#endif

ILidarDriver* HelloWorld::lidar_drv = nullptr;

HelloWorld::HelloWorld():
    opt_channel_type(CHANNEL_TYPE_SERIALPORT)
{
}

HelloWorld::~HelloWorld()
{
}

Scene* HelloWorld::createScene()
{
    return HelloWorld::create();
}


bool checkSLAMTECLIDARHealth(ILidarDriver* drv)
{
    sl_result     op_result;
    sl_lidar_response_device_health_t healthinfo;

    op_result = drv->getHealth(healthinfo);
    if (SL_IS_OK(op_result)) { // the macro IS_OK is the preperred way to judge whether the operation is succeed.
        printf("SLAMTEC Lidar health status : %d\n", healthinfo.status);
        if (healthinfo.status == SL_LIDAR_STATUS_ERROR) {
            fprintf(stderr, "Error, slamtec lidar internal error detected. Please reboot the device to retry.\n");
            // enable the following code if you want slamtec lidar to be reboot by software
            // drv->reset();
            return false;
        }
        else {
            return true;
        }

    }
    else {
        fprintf(stderr, "Error, cannot retrieve the lidar health code: %x\n", op_result);
        return false;
    }
}

// Print useful error message instead of segfaulting when files are not there.
static void problemLoading(const char* filename)
{
    printf("Error while loading: %s\n", filename);
    printf("Depending on how you compiled you might have to add 'Resources/' in front of filenames in HelloWorldScene.cpp\n");
}

// on "init" you need to initialize your instance
bool HelloWorld::init()
{
    //////////////////////////////
    // 1. super init first
    if ( !Scene::init() )
    {
        return false;
    }

    radarScanViewLayer = RadarScanViewLayer::create();
    this->addChild(radarScanViewLayer);

    controlRadar = ControlRadar::create();
    this->addChild(controlRadar);

    return true;
}

void HelloWorld::createButton(const Vec2& pos, std::string& txt, int id)
{
    auto button = ui::Button::create("CloseNormal.png", "CloseSelected.png");
    button->setTitleText(txt);
    button->setPosition(pos);
    button->setScale(2.f);
    button->addTouchEventListener([=](Ref* sender, ui::Widget::TouchEventType type)
        {
            switch (type)
            {
            case cocos2d::ui::Widget::TouchEventType::BEGAN:
                break;
            case cocos2d::ui::Widget::TouchEventType::ENDED:
                switch (id)
                {
                case 0:
                {                    
                    auto c = ConnectRadar::getInstance()->onConnect();
                /*beginRadar();*/
                }
                    break;
                case 1:
                    stopRadar();
                    break;
                case 2:
                    resumeRadar();
                    break;
                case 3:
                    pauseRadar();
                    break;
                case 4:
                    break;
                }
                break;
            }
        });

    this->addChild(button);
}

void HelloWorld::beginRadar()
{
    const char* opt_is_channel = NULL;
    const char* opt_channel = NULL;
    const char* opt_channel_param_first = NULL;
    sl_u32         opt_channel_param_second = 0;
    sl_u32         baudrateArray[2] = { 115200, 256000 };
    /*sl_result     op_result*/;
    /* int          opt_channel_type = CHANNEL_TYPE_SERIALPORT;*/

    bool useArgcBaudrate = false;

    IChannel* _channel;

    printf("Ultra simple LIDAR data grabber for SLAMTEC LIDAR.\n"
        "Version: %s\n", "SL_LIDAR_SDK_VERSION");

    auto argc = 4;
    std::string argv[4] = { {""}, {"--channel"},{"-s"},{"\\\\.\\com3"} };

    if (argc > 1)
    {
        opt_is_channel = argv[1].c_str();
    }
    else
    {
        return;
    }

    if (strcmp(opt_is_channel, "--channel") == 0) {
        opt_channel = argv[2].c_str();
        if (strcmp(opt_channel, "-s") == 0 || strcmp(opt_channel, "--serial") == 0)
        {
            // read serial port from the command line...
            opt_channel_param_first = argv[3].c_str();// or set to a fixed value: e.g. "com3"
            // read baud rate from the command line if specified...
            if (argc > 4)
            {
                opt_channel_param_second = strtoul(argv[4].c_str(), NULL, 10);
                useArgcBaudrate = true;
            }
        }
        else if (strcmp(opt_channel, "-u") == 0 || strcmp(opt_channel, "--udp") == 0)
        {
            // read ip addr from the command line...
            opt_channel_param_first = argv[3].c_str();//or set to a fixed value: e.g. "192.168.11.2"
            if (argc > 4) opt_channel_param_second = strtoul(argv[4].c_str(), NULL, 10);//e.g. "8089"
            opt_channel_type = CHANNEL_TYPE_UDP;
        }
        else
        {
            return;
        }
    }
    else
    {
        return;
    }

    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT)
    {
        if (!opt_channel_param_first) {
#ifdef _WIN32
            // use default com port
            opt_channel_param_first = "\\\\.\\com3";
#elif __APPLE__
            opt_channel_param_first = "/dev/tty.SLAB_USBtoUART";
#else
            opt_channel_param_first = "/dev/ttyUSB0";
#endif
        }
    }


    // create the driver instance
    if (!lidar_drv)
    {
        lidar_drv = *createLidarDriver();
    }

    if (!lidar_drv) {
        fprintf(stderr, "insufficent memory, exit\n");
        exit(-2);
    }

    sl_lidar_response_device_info_t devinfo;
    bool connectSuccess = false;

    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT) {
        if (useArgcBaudrate) {
            _channel = (*createSerialPortChannel(opt_channel_param_first, opt_channel_param_second));
            if (SL_IS_OK((lidar_drv)->connect(_channel))) {
                op_result = lidar_drv->getDeviceInfo(devinfo);

                if (SL_IS_OK(op_result))
                {
                    connectSuccess = true;
                }
                else {
                    delete lidar_drv;
                    lidar_drv = NULL;
                }
            }
        }
        else {
            size_t baudRateArraySize = (sizeof(baudrateArray)) / (sizeof(baudrateArray[0]));
            for (size_t i = 0; i < baudRateArraySize; ++i)
            {
                _channel = (*createSerialPortChannel(opt_channel_param_first, baudrateArray[i]));
                if (SL_IS_OK((lidar_drv)->connect(_channel))) {
                    op_result = lidar_drv->getDeviceInfo(devinfo);

                    if (SL_IS_OK(op_result))
                    {
                        connectSuccess = true;
                        break;
                    }
                    else {
                        delete lidar_drv;
                        lidar_drv = NULL;
                    }
                }
            }
        }
    }
    else if (opt_channel_type == CHANNEL_TYPE_UDP) {
        _channel = *createUdpChannel(opt_channel_param_first, opt_channel_param_second);
        if (SL_IS_OK((lidar_drv)->connect(_channel))) {
            op_result = lidar_drv->getDeviceInfo(devinfo);

            if (SL_IS_OK(op_result))
            {
                connectSuccess = true;
            }
            else {
                delete lidar_drv;
                lidar_drv = NULL;
            }
        }
    }


    if (!connectSuccess) {
        (opt_channel_type == CHANNEL_TYPE_SERIALPORT) ?
            (fprintf(stderr, "Error, cannot bind to the specified serial port %s.\n"
                , opt_channel_param_first)) : (fprintf(stderr, "Error, cannot connect to the specified ip addr %s.\n"
                    , opt_channel_param_first));

        stopRadar();
    }

    // print out the device serial number, firmware and hardware version number..
    CCLOG("SLAMTEC LIDAR S/N: ");
    for (int pos = 0; pos < 16; ++pos) {
        CCLOG("%02X", devinfo.serialnum[pos]);
    }

    CCLOG("\n"
        "Firmware Ver: %d.%02d\n"
        "Hardware Rev: %d\n"
        , devinfo.firmware_version >> 8
        , devinfo.firmware_version & 0xFF
        , (int)devinfo.hardware_version);



    // check health...
    if (!checkSLAMTECLIDARHealth(lidar_drv)) {
        stopRadar();
    }

    // signal(SIGINT, ctrlc);
}

void HelloWorld::stopRadar()
{
    this->unschedule("info");

    lidar_drv->stop();
    delay(200);
    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT)
    {
        lidar_drv->setMotorSpeed(0);
    }

    /*if (lidar_drv) {
        delete lidar_drv;
        lidar_drv = nullptr;
    }*/
}

void HelloWorld::pauseRadar()
{
    this->unschedule("info");

    lidar_drv->stop();
    delay(200);
    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT)
    {
        lidar_drv->setMotorSpeed(0);
    }
}

void HelloWorld::resumeRadar()
{
    if (opt_channel_type == CHANNEL_TYPE_SERIALPORT)
        lidar_drv->setMotorSpeed();
    // start scan...
    lidar_drv->startScan(0, 1);

    schedule([&](float delta) { getInformation(); }, 1.f, "info");
}

void HelloWorld::getInformation()
{
    sl_lidar_response_measurement_node_hq_t nodes[8192];
    size_t   count = _countof(nodes);

    op_result = lidar_drv->grabScanDataHq(nodes, count);

    if (SL_IS_OK(op_result)) {
        lidar_drv->ascendScanData(nodes, count);
        for (int pos = 0; pos < (int)count; ++pos) {
            CCLOG("%s theta: %03.2f Dist: %08.2f Q: %d \n",
                (nodes[pos].flag & SL_LIDAR_RESP_HQ_FLAG_SYNCBIT) ? "S " : "  ",
                (nodes[pos].angle_z_q14 * 90.f) / 16384.f,
                nodes[pos].dist_mm_q2 / 4.0f,
                nodes[pos].quality >> SL_LIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);
        }
    }
}
