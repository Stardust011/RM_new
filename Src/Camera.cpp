//
// Created by Stardust on 2023/12/17.
//

#include "Camera.h"
#include "config.h"



// auto resolution = config["camera"]["resolution"].as<int>();
// auto frame_rate = config["camera"]["frame_rate"].as<int>();
auto device = config["camera"]["device"].as<std::string>();

auto exposure = config["camera"]["exposure"].as<std::string>();

cv::VideoCapture cap_test;

// 相机初始化
void Camera_Init() {
    // Open the camera device
    if (std::all_of(device.begin(), device.end(), ::isdigit)) {
        cap_test.open(std::stoi(device));
    } else {
        cap_test.open(device);
    }

    // Set the camera parameters
    // cap.set(cv::CAP_PROP_FRAME_WIDTH, resolution);
    // cap.set(cv::CAP_PROP_FRAME_HEIGHT, resolution);
    // cap.set(cv::CAP_PROP_FPS, frame_rate);
    if (exposure == "auto") {
        cap_test.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
    } else {
        cap_test.set(cv::CAP_PROP_AUTO_EXPOSURE, 0);
        cap_test.set(cv::CAP_PROP_EXPOSURE, std::stoi(exposure));
    }
    setCameraExposure(exposure);
    cap_test.release();
}

bool testCamera() {
    cv::Mat frame;
    setOpencvDevice(device);
    if (std::all_of(device.begin(), device.end(), ::isdigit)) {
        cap_test.open(std::stoi(device));
    } else {
        cap_test.open(device);
    }
    // sleep(3);
    if (!cap_test.isOpened()) {
        // std::cout << "Camera open failed!" << std::endl;
        setCameraStatus("Camera open failed!");
        return false;
    }
    cap_test >> frame;
    if (frame.empty()) {
        // std::cout << "Camera read failed!" << std::endl;
        setCameraStatus("Camera read failed!");
        return false;
    }
    setCameraStatus("Camera Connected");
    cap_test.release();
    return true;
}
