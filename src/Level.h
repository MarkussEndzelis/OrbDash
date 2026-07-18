#pragma once
#include <vector>
#include "Obstacle.h"

class Level {
public:
    static Level testLevel();

    const std::vector<Obstacle>& getObstacles() const { return obstacles;}
    float getLength() const { return length; }

private:
    std::vector<Obstacle> obstacles;
    float length;
};