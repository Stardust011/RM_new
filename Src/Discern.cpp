//
// Created by Stardust on 2023/12/17.
//

#include "Discern.h"
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "config.h"
#include "SerialPortCom.h"

// 获取摄像头参数
auto camera_device = config["camera"]["device"].as<std::string>();

// 设置摄像头对象
cv::VideoCapture cap(camera_device);

cv::Mat readFrame(cv::VideoCapture cap)
{
    cv::Mat frame;
    cap >> frame;
    return frame;
}

// 预处理提取红色区域
cv::Mat preprocess(const cv::Mat& frame)
{
    cv::Mat red;
    // 转换为HSV空间
    cv::cvtColor(frame, red, cv::COLOR_BGR2HSV);
    // 提取红色区域
    cv::Mat mask1, mask2;
    cv::inRange(red, cv::Scalar(156, 43, 46), cv::Scalar(180, 255, 255), mask1);
    cv::inRange(red, cv::Scalar(0, 43, 46), cv::Scalar(10, 255, 255), mask2);
    red = mask1 + mask2;

    cv::GaussianBlur(red, red, cv::Size(5, 5), 1.5);
    return red;
}

// 寻找圆形区域
void findCircle(const cv::Mat& input) {
    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(input, circles, cv::HOUGH_GRADIENT, 1, 10, 100, 30, 5, 100);
    // 判断是否存在圆
    if(circles.empty())
    {
        setDiscernStatus("No circle");
        setCmdStatus(0);
        return;
    }
    setCmdStatus(1);
    setDiscernStatus("Circle found");
    // 寻找最靠近画面中心的圆形
    int min = 10000;
    int index = 0;
    for(int i = 0; i < circles.size(); i++)
    {
        const int x = cvRound(circles[i][0]);
        //        int y = cvRound(circles[i][1]);
        //        int r = cvRound(circles[i][2]);
        if(abs(x - input.cols / 2) < min)
        {
            min = abs(x - input.cols / 2);
            index = i;
        }
    }
    const double x = circles[index][0]-input.cols/2;
    const double y = circles[index][1]-input.rows/2;
    setDiscernDirection(circles[index][0], circles[index][1], circles[index][2]);
    // 发送数据
    setSerialData(x, y);
}

// 识别程序入口（多线程）
[[noreturn]] void Discern_run() {
    //定义clock_t变量
    while (true) {
        const clock_t start = clock();//开始时间
        cv::Mat frame = readFrame(cap);
        frame = preprocess(frame);
        findCircle(frame);
        // sleep(1);
        // setSerialData(i++, (double)2);
        // std::cout << "Discern_run" << std::endl;
        const clock_t end = clock();//结束时间
        setTimeCost(static_cast<double>(end - start)/CLOCKS_PER_SEC);
        // std::cout << "Discern_run time: " << (double)(end-start)/CLOCKS_PER_SEC << std::endl;
    }
}
