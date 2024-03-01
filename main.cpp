//
//          ___  ____   __    ____  ____  __  __  ___  ____  ___
//         / __)(_  _) /__\  (  _ \(  _ \(  )(  )/ __)(_  _)/ __)
//         \__ \  )(  /(__)\  )   / )(_) ))(__)( \__ \  )(  \__ \
//         (___/ (__)(__)(__)(_)\_)(____/(______)(___/ (__) (___/


#include <thread>
#include "SerialPortCom.h"
#include "Camera.h"
#include "Discern.h"
#include <filesystem>
#include "config.h"
#include "tui.h"
#include <iostream>

bool configFileExists(const std::string& filename = CONFIG_PATH) {
    return std::filesystem::exists(filename);
}

int precheck() {
    if (!configFileExists()) {
        std::cout << "Config file not found!" << std::endl;
        std::cout << "Please check the config file path.(" << CONFIG_PATH << ")" << std::endl;
        // 结束所有程序
        exit(EXIT_FAILURE);
    }else{
        std::cout << "Config file found!" << std::endl;
    }
    return 0;
}

int main()
{
    precheck();
    std::thread thread1(displayStatus);
    std::thread thread2(serialSendEntry);
    std::thread thread3(Discern_run);
    //
    // 确保在程序结束前等待线程完成
    thread1.join();
    thread2.join();
    thread3.join();

    return 0;
}
