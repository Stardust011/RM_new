#include <thread>
#include "SerialPortCom.h"
#include "Camera.h"
#include "Discern.h"
#include <filesystem>
#include "config.h"
#include "tui.h"

bool configFileExists(const std::string& filename = CONFIG_PATH) {
    return std::filesystem::exists(filename);
}

int precheck() {
    if (!configFileExists()) {
        std::cout << "Config file not found!" << std::endl;
        std::cout << "Please check the config file path.(./config.yaml)" << std::endl;
        // 结束所有程序
        exit(EXIT_FAILURE);
    }
    if (!testCamera()) {
        exit(EXIT_FAILURE);
    }
    return 0;
}

int main()
{
    std::thread thread3(displayStatus);
    if (DEVELOPE) {
        std::cout << "DEVELOPE" << std::endl;
    } else {
        std::cout << "RELEASE" << std::endl;
        precheck();
    }
    testCamera();
    Camera_Init();
    std::thread thread1(serialSendEntry);
    std::thread thread2(Discern_run);

    // 确保在程序结束前等待线程完成
    thread1.join();
    thread2.join();
    thread3.join();

    return 0;
}
