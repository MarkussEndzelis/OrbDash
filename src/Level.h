#pragma once
#include <vector>
#include <string>
#include "Obstacle.h"

class Level {
public:
    static Level testLevel();
    static Level loadFromFile(const std::string& path);

    const std::vector<Obstacle>& getObstacles() const { return obstacles;}
    float getLength() const { return length; }

private:
    std::vector<Obstacle> obstacles;
    float length;
};