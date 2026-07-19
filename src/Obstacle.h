#pragma once

enum class ObstacleType {Block, Spike, Platform};

struct Obstacle{
    ObstacleType type;
    float worldX;
    float width;
    float height;
};