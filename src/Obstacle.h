#pragma once

enum class ObstacleType {Block, Spike};

struct Obstacle{
    ObstacleType type;
    float worldX;
    float width;
    float height;
};