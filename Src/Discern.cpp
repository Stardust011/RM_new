//
// Created by Stardust on 2023/12/17.
//

#include "Discern.h"
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>

#include "Camera.h"
#include "config.h"
#include "SerialPortCom.h"



// 识别程序入口（多线程）
void Discern_run() {
    double i=0;
    while (1) {
        sleep(1);
        setSerialData(i++, (double)2);
        // std::cout << "Discern_run" << std::endl;
    }
}
