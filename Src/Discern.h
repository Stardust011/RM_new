//
// Created by Stardust on 2023/12/17.
//
#pragma once
#ifndef DISCERN_H
#define DISCERN_H
#include <iostream>
#include <unistd.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

enum ColorMode {
    MODE_RED,
    MODE_BLUE,
    MODE_NONE
};

void Discern_run();

#endif //DISCERN_H
