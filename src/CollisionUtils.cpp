#include "CollisionUtils.h"
#include <algorithm>
#include <cmath>

namespace {
    float clamp(float val, float min, float max){
        return std::max(min, std::min(max, val));
    }

    float sign(float px, float py, float x1, float y1, float x2, float y2){
        return (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2);
    }

    bool pointInTriangle(float px, float py, float x1, float y1, float x2, float y2, float x3, float y3){
        float d1 = sign(px, py, x1, y1, x2, y2);
        float d2 = sign(px, py, x2, y2, x3, y3);
        float d3 = sign(px, py, x3, y3, x1, y1);

        bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

        return !(hasNeg && hasPos);
    }

    bool circleSegmentCollides(float cx, float cy, float radius, float x1, float y1, float x2, float y2){
        float closestX, closestY;
        float dx = x2 - x1, dy = y2 - y1;
        float lengthSquared = dx * dx + dy * dy;

        if (lengthSquared == 0.f){
            closestX = x1;
            closestY = y1;
        }else {
            float t = ((cx - x1) * dx + (cy - y1) * dy) / lengthSquared;
            t = clamp(t, 0.f, 1.f);
            closestX = x1 + t * dx;
            closestY = y1 + t * dy;
        }

        float distX = cx - closestX;
        float distY = cy - closestY;
        return (distX * distX + distY * distY) < (radius * radius);
    }
}

namespace CollisionUtils {

bool circleRectCollides(float cx, float cy, float radius, float baseX, float baseY, float width, float height) {
    float x1 = baseX, y1 = baseY;
    float x2 = baseX + width, y2 = baseY;
    float x3 = baseX + width / 2.f, y3 = baseY - height;

    if (pointInTriangle(cx, cy, x1, y1, x2, y2, x3, y3)){
        return true;
    }

    return circleSegmentCollides(cx, cy, radius, x1, y1, x2, y2) || circleSegmentCollides(cx, cy, radius, x2, y2, x3, y3) || circleSegmentCollides(cx, cy, radius, x3, y3, x1, y1);
}
}