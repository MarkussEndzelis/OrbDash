#include "Level.h"

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