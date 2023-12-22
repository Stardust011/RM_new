//
// Created by Stardust on 2023/12/21.
//

#include "tui.h"
#include <thread>
#include <utility>
#include <sstream>
#include <iomanip>
#include <curses.h>
using namespace std;

string opencv_device = "Finding...";
string camera_exposure = "Finding...";
string serial_device = "Finding...";
string camera_status = "Finding...";
string discern_status = "Finding...";
string serial_status = "Finding...";
string serial_baudrate = "Finding...";
string serial_send = "Finding...";
string discern_direction = "Finding...";

void setOpencvDevice(string device) {
    opencv_device = std::move(device);
}

void setCameraExposure(string exposure) {
    camera_exposure = std::move(exposure);
}

void setSerialDevice(string device) {
    serial_device = std::move(device);
}

void setCameraStatus(string status) {
    camera_status = std::move(status);
}

void setDiscernStatus(string status) {
    discern_status = std::move(status);
}

void setSerialStatus(string status) {
    serial_status = std::move(status);
}

void setSerialBaudrate(string baudrate) {
    serial_baudrate = std::move(baudrate);
}

void setSerialSend(int send_x, int send_y) {
    // 转换为字符串,补足零为三位整数
    ostringstream out_x;
    out_x << fixed << setprecision(0) << setw(5)  << send_x;
    ostringstream out_y;
    out_y << fixed << setprecision(0) << setw(5)  << send_y;
    serial_send = "( x:" + out_x.str() + ", y:" + out_y.str() + " )";
}

// TODO:not string
void setDiscernDirection(string direction) {
    discern_direction = std::move(direction);
}

void displayStatus() {
    // 初始化ncurses
    initscr();
    // 不显示输入的字符
    noecho();

    // 获取窗口的大小
    int height, width;
    getmaxyx(stdscr, height, width);
    int half_height = height / 2;
    if (half_height < 6) { half_height = 6;} // 防止窗口过小
    int half_width = width / 2;

    // 添加分割线
    mvhline(half_height, 0, ACS_HLINE, width);

    while (1) {
        // 在窗口中显示一些文本

        // 分割线上方
        // 摄像头地址
        mvprintw(1, 1, ("Opencv device: " + opencv_device).c_str());
        // 摄像头状态
        // 清除上一次的内容
        move(2,1);
        clrtoeol();
        mvprintw(2, 1, ("Camera status: " + camera_status).c_str());
        // 摄像头曝光度
        mvprintw(3, 1, ("Camera exposure: " + camera_exposure).c_str());
        // 识别状态
        mvprintw(4, 1, ("Discern status: " + discern_status).c_str());
        // 识别方位
        mvprintw(5, 1, ("Discern direction: " + discern_direction).c_str());

        // 分割线下方
        // 串口地址
        mvprintw(half_height+1, 1, ("Serial Port device: " + serial_device).c_str());
        // 串口波特率
        // 清除上一次的内容
        move(half_height+2,1);
        clrtoeol();
        mvprintw(half_height+2, 1, ("Serial Port baudrate: " + serial_baudrate).c_str());
        // 串口状态
        mvprintw(half_height+3, 1, ("Serial Port status: " + serial_status).c_str());
        // 串口发送数据
        mvprintw(half_height+4, 1, ("Serial Port send: " + serial_send).c_str());

        // 最后两行
        // 按键提示
        mvprintw(half_height+6, 1, "Press 'Ctrl + C' to quit.");
        refresh();
        // 每10毫秒刷新一次
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 等待用户输入
    getch();

    // 删除窗口并结束ncurses
    endwin();
}
