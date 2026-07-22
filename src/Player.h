#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class Player {
public:
    static constexpr float RADIUS = 18.f;

    Player();

    void reset();
    void jump();
    void update(float dt, float floorY);
    void render(sf::RenderWindow& window, float screenX);

    float getY() const { return y; }
    bool isOnGround() const { return onGround; }
    static float getGroundY();
    void setSkin(sf::Color color);

private:
    struct TrailPoint{float y; float age; };
    std::vector<TrailPoint> trail;
    sf::Color skinColor = sf::Color(0x5e, 0xc4, 0xff);
    static constexpr float TRAIL_LIFETIME = 0.35f;
    static constexpr float TRAIL_SPEED = 360.f;

    float y;
    float velocityY;
    float rotation;
    bool onGround;

    static constexpr float GRAVITY = 2200.f;
    static constexpr float JUMP_VELOCITY = -820.f;
};