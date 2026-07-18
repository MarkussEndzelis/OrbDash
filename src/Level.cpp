#include "Level.h"
#include <fstream>
#include <sstream>
#include <iostream>

Level Level::testLevel() {
    Level level;
    float spikeSize = 40.f;
    float blockSize = 40.f;

    level.obstacles = {
        { ObstacleType::Spike, 600.f, spikeSize, spikeSize},
        { ObstacleType::Spike, 900.f, spikeSize, spikeSize},
        { ObstacleType::Block, 1200.f, blockSize, blockSize},
        { ObstacleType::Spike, 1500.f, spikeSize, spikeSize},
        { ObstacleType::Spike, 1560.f, spikeSize, spikeSize},
        { ObstacleType::Block, 1900.f, blockSize, blockSize},
        { ObstacleType::Block, 1970.f, blockSize, blockSize},
        { ObstacleType::Spike, 2300.f, spikeSize, spikeSize},
    };

    level.length = 2600.f;
    return level;
}

Level Level::loadFromFile(const std::string& path){
    Level level;
    std::ifstream file(path);

    if (!file.is_open()){
        std::cerr << "Failed to open level file: " << path << " - falling back to test level.\n";
        return testLevel();
    }

    std::string line;
    while (std::getline(file, line)){
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string keyword;
        iss >> keyword;

        if (keyword == "LENGTH"){
            iss >> level.length;
        }else if (keyword == "SPIKE"){
            float x;
            iss >> x;
            level.obstacles.push_back({ ObstacleType::Spike, x, 40.f, 40.f});
        }else if (keyword == "BLOCK"){
            float x;
            float size = 40.f;
            iss >> x;
            if (!(iss >> size)){
                size = 40.f;
            }
            level.obstacles.push_back({ ObstacleType::Block, x, size, size});
        }
    }

    if (level.length <= 0.f && !level.obstacles.empty()){
        level.length = level.obstacles.back().worldX + 300.f;
    }
    return level;
}