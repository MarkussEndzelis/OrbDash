#include "Player.h"
#include "Game.h"

Player::Player(){
    reset();
}

void Player::reset(){
    y = getGroundY() - RADIUS;
    velocityY = 0.f;
    rotation = 0.f;
    onGround = true;
}

void Player::jump(){
    if (onGround){
        velocityY = JUMP_VELOCITY;
        onGround = false;
    }
}

void Player::update(float dt){
    velocityY += GRAVITY * dt;
    y += velocityY * dt;

    float floor = getGroundY() - RADIUS;
    if (y >= floor){
        y = floor;
        velocityY = 0.f;
        onGround = true;
    }else {
        onGround = false;
    }

    if (!onGround) {
        rotation += 480.f * dt;
    }else {
        rotation = std::round(rotation / 90.f) * 90.f;
    }
}

float Player::getGroundY(){
    return Game::HEIGHT - 80.f;
}

void Player::render(sf::RenderWindow& window, float screenX){
    sf::CircleShape shape(RADIUS);
    shape.setOrigin(RADIUS, RADIUS);
    shape.setPosition(screenX, y);
    shape.setRotation(rotation);
    shape.setFillColor(sf::Color(0x5e, 0xc4, 0xff));
    window.draw(shape);

    sf::RectangleShape indicator(sf::Vector2f(RADIUS, 3.f));
    indicator.setOrigin(0.f, 1.5f);
    indicator.setPosition(screenX, y);
    indicator.setRotation(rotation);
    indicator.setFillColor(sf::Color(0x1b, 0x1b, 0x2b));
    window.draw(indicator);
}