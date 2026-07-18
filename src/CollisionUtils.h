#pragma once

namespace CollisionUtils {
    bool circleRectCollides(float cx, float cy, float radius, float rx, float ry, float rw, float rh);

    bool circleTriangleCollides(float cx, float cy, float radius, float baseX, float baseY, float width, float height);
}