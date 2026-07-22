#include "Player.h"
#include "Game.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

Player::Player() {
    reset();
}

void Player::reset(){
    y = getGroundY() - RADIUS;
    velocityY = 0.f;
    rotation = 0.f;
    onGround = true;
    trail.clear();
}

void Player::setSkin(sf::Color color){
    skinColor = color;
}

void Player::jump() {
    if (onGround) {
        velocityY = JUMP_VELOCITY;
        onGround = false;
    }
}

void Player::update(float dt, float floorY) {
    velocityY += GRAVITY * dt;
    y += velocityY * dt;

    float floor = floorY - RADIUS;
    if (y >= floor) {
        y = floor;
        velocityY = 0.f;
        onGround = true;
    } else {
        onGround = false;
    }

    if (!onGround) {
        rotation += 480.f * dt;
        trail.push_back({y, 0.f});
    }else {
        rotation = std::round(rotation / 90.f) * 90.f;
    }

    for (auto& t : trail) t.age += dt;
    trail.erase(std::remove_if(trail.begin(), trail.end(), 
            [](const TrailPoint& t){return t.age > TRAIL_LIFETIME; }), trail.end());
}

float Player::getGroundY() {
    return Game::HEIGHT - 80.f;
}

void Player::render(sf::RenderWindow& window, float screenX) {
    for (const auto& t : trail){
        float alpha = std::max(0.f, 1.f - (t.age / TRAIL_LIFETIME));
        float trailX = screenX - t.age * TRAIL_SPEED;
        sf::CircleShape trailDot(RADIUS * 0.7f);
        trailDot.setOrigin(sf::Vector2f(RADIUS * 0.7f, RADIUS * 0.7f));
        trailDot.setPosition(sf::Vector2f(trailX, t.y));
        sf::Color tc = skinColor;
        tc.a = (std::uint8_t)(alpha * 120.f);
        trailDot.setFillColor(tc);
        window.draw(trailDot);
    }

    sf::CircleShape shape(RADIUS);
    shape.setOrigin(sf::Vector2f(RADIUS, RADIUS));
    shape.setPosition(sf::Vector2f(screenX, y));
    shape.setRotation(sf::degrees(rotation));
    shape.setFillColor(skinColor);
    window.draw(shape);

    sf::RectangleShape indicator(sf::Vector2f(RADIUS, 3.f));
    indicator.setOrigin(sf::Vector2f(0.f, 1.5f));
    indicator.setPosition(sf::Vector2f(screenX, y));
    indicator.setRotation(sf::degrees(rotation));
    indicator.setFillColor(sf::Color(0x1b, 0x1b, 0x2b));
    window.draw(indicator);
}