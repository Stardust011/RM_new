//
// Created by Stardust on 2023/12/21.
//

#ifndef TUI_H
#define TUI_H

#include <string>

void displayStatus();
void setOpencvDevice(std::string device);
void setCameraExposure(std::string exposure);
void setSerialDevice(std::string device);
void setCameraStatus(std::string status);
void setDiscernStatus(std::string status);
void setSerialStatus(std::string status);
void setSerialBaudrate(std::string baudrate);
void setSerialSend(int send_x, int send_y);

// TODO:not string
void setDiscernDirection(std::string direction);

#endif //TUI_H