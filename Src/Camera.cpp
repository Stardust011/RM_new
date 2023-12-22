//
// Created by Stardust on 2023/12/17.
//

#include "Camera.h"

#include <unistd.h>

#include "config.h"
#include <opencv4/opencv2/opencv.hpp>

// auto resolution = config["camera"]["resolution"].as<int>();
// auto frame_rate = config["camera"]["frame_rate"].as<int>();
auto device = config["camera"]["device"].as<std::string>();
auto exposure = config["camera"]["exposure"].as<std::string>();

cv::VideoCapture cap;

void Camera_Init() {
    // Open the camera device
    cap.open(device);

    // Set the camera parameters
    // cap.set(cv::CAP_PROP_FRAME_WIDTH, resolution);
    // cap.set(cv::CAP_PROP_FRAME_HEIGHT, resolution);
    // cap.set(cv::CAP_PROP_FPS, frame_rate);
    if (exposure == "auto") {
        cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    } else {
        cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0);
        cap.set(cv::CAP_PROP_EXPOSURE, std::stoi(exposure));
    }
    cap.release();
}

bool testCamera() {
    cv::Mat frame;
    setOpencvDevice(device);
    cap.open(device);
    // sleep(3);
    if (!cap.isOpened()) {
        // std::cout << "Camera open failed!" << std::endl;
        setCameraStatus("Camera open failed!");
        return false;
    }
    cap >> frame;
    if (frame.empty()) {
        // std::cout << "Camera read failed!" << std::endl;
        setCameraStatus("Camera read failed!");
        return false;
    }
    setCameraStatus("Camera Connected");
    cap.release();
    return true;
}