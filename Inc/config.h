//
// Created by Stardust on 2023/12/17.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>
#include "tui.h"

#define DEVELOPE 1

#ifdef DEVELOPE
#define CONFIG_PATH "./config.yaml"
#else
#define CONFIG_PATH "config.yaml"
#endif

inline YAML::Node config = YAML::LoadFile(CONFIG_PATH);

#endif //CONFIG_H
