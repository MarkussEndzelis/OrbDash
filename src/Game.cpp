#include "Game.h"
#include "CollisionUtils.h"
#include <sstream>
#include <iomanip>

Game::Game()
    : window(sf::VideoMode(sf::Vector2u((unsigned int)WIDTH, (unsigned int)HEIGHT)), "OrbDash"),
      level(Level::testLevel())
{
    window.setFramerateLimit(144);
    fontLoaded = font.openFromFile("assets/font.ttf");

    levels = {
        { "Level 1 - Warmup", "assets/levels/level1.txt"},
        { "Level 2 - Rush", "assets/levels/level2.txt"}
    };
}

void Game::startLevel(const std::string& path){
    level = Level::loadFromFile(path);
    player.reset();
    cameraX = 0.f;
    dead = false;
    won = false;
    started = false;
    state = GameState::Playing;
}

void Game::restart() {
    player.reset();
    cameraX = 0.f;
    dead = false;
    won = false;
    started = false;
}

sf::FloatRect Game::playButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(WIDTH / 2.f - 100.f, HEIGHT/ 2.f - 30.f), sf::Vector2f(200.f, 60.f));
}
std::vector<sf::FloatRect> Game::levelButtonBounds() const {
    std::vector<sf::FloatRect> bounds;
    float boxWidth = 260.f;
    float boxHeight = 80.f;
    float spacing = 30.f;
    float totalWidth = levels.size() * boxWidth + (levels.size() - 1) * spacing;
    float startX = WIDTH / 2.f - totalWidth / 2.f;
    float y = HEIGHT / 2.f - boxHeight / 2.f;

    for (size_t i = 0; i < levels.size(); i++){
        float x = startX + i * (boxWidth + spacing);
        bounds.push_back(sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(boxWidth, boxHeight)));
    }
    return bounds;
}

void Game::handleMainMenuClick(sf::Vector2f mousePos){
    if (playButtonBounds().contains(mousePos)){
        state = GameState::LevelSelect;
    }
}

void Game::handleLevelSelectClick(sf::Vector2f mousePos){
    auto bounds = levelButtonBounds();
    for (size_t i = 0; i < bounds.size(); i++){
        if (bounds[i].contains(mousePos)){
            startLevel(levels[i].path);
            return;
        }
    }
}

void Game::handleInput() {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
        }
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if(state == GameState::Playing){
                if (keyPressed->code == sf::Keyboard::Key::Space) {
                    if (dead || won) {
                        state = GameState::LevelSelect;
                    }else if(!started){
                        started = true;
                    }else{
                        player.jump();
                    }
                }
                if (keyPressed->code == sf::Keyboard::Key::Escape){
                    state = GameState::LevelSelect;
                }
            }
        }
        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()){
            sf::Vector2f mousePos((float)mousePressed->position.x, (float)mousePressed->position.y);

            if (state == GameState::MainMenu){
                handleMainMenuClick(mousePos);
            }else if (state == GameState::LevelSelect){
                handleLevelSelectClick(mousePos);
            }else if (state == GameState::Playing){
                if (dead || won){
                    state = GameState::LevelSelect;
                }else if(!started){
                    started = true;
                }else {
                    player.jump();
                }
            }
        }
    }
}

void Game::update(float dt) {
    if (state != GameState::Playing) return;
    if (dead || !started) return;

    cameraX += SCROLL_SPEED * dt;
    player.update(dt);

    float playerWorldX = 150.f + cameraX;
    for (const auto& obs : level.getObstacles()) {
        bool hit;
        if (obs.type == ObstacleType::Block) {
            float top = Player::getGroundY() - obs.height;
            hit = CollisionUtils::circleRectCollides(playerWorldX, player.getY(), Player::RADIUS,
                    obs.worldX, top, obs.width, obs.height);
        } else {
            hit = CollisionUtils::circleTriangleCollides(playerWorldX, player.getY(), Player::RADIUS,
                    obs.worldX, Player::getGroundY(), obs.width, obs.height);
        }
        if (hit) {
            dead = true;
            break;
        }
    }

    if (cameraX >= level.getLength()) {
        won = true;
        started = false;
    }
}

void Game::renderMainMenu(){
    if (!fontLoaded) return;

    sf::Text title(font, "OrbDash", 48);
    title.setFillColor(sf::Color::White);
    title.setPosition(sf::Vector2f(WIDTH / 2.f - 100.f, HEIGHT / 2.f - 140.f));
    window.draw(title);

    sf::FloatRect btn = playButtonBounds();
    sf::RectangleShape btnShape(btn.size);
    btnShape.setPosition(btn.position);
    btnShape.setFillColor(sf::Color(0x5e, 0xc4, 0xff));
    window.draw(btnShape);

    sf::Text playText(font, "PLAY", 28);
    playText.setFillColor(sf::Color(0x1b, 0x1b, 0x2b));
    playText.setPosition(sf::Vector2f(btn.position.x + btn.size.x / 2.f - 35.f, btn.position.y + 12.f));
    window.draw(playText);
}

void Game::renderLevelSelect(){
    if (!fontLoaded) return;

    sf::Text title(font, "Select a level", 36);
    title.setFillColor(sf::Color::White);
    title.setPosition(sf::Vector2f(WIDTH / 2.f - 150.f, 80.f));
    window.draw(title);

    auto bounds = levelButtonBounds();
    for (size_t i = 0; i < bounds.size(); i++){
        sf::RectangleShape box(bounds[i].size);
        box.setPosition(bounds[i].position);
        box.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
        box.setOutlineColor(sf::Color(0xe8, 0xa3, 0x3d));
        box.setOutlineThickness(2.f);
        window.draw(box);

        sf::Text label(font, levels[i].name, 18);
        label.setFillColor(sf::Color::White);
        label.setPosition(sf::Vector2f(bounds[i].position.x + 15.f, bounds[i].position.y + 30.f));
        window.draw(label);
    }
}

void Game::renderPlaying(){
    sf::RectangleShape ground(sf::Vector2f(WIDTH, HEIGHT - Player::getGroundY()));
    ground.setPosition(sf::Vector2f(0.f, Player::getGroundY()));
    ground.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(ground);

    for (const auto& obs : level.getObstacles()){
        float screenX = obs.worldX - cameraX;
        if (screenX < -100.f || screenX > WIDTH + 100.f) continue;

        if (obs.type == ObstacleType::Block){
            sf::RectangleShape rect(sf::Vector2f(obs.width, obs.height));
            rect.setPosition(sf::Vector2f(screenX, Player::getGroundY() - obs.height));
            rect.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(rect);
        }else {
            sf::ConvexShape tri(3);
            float baseY = Player::getGroundY();
            tri.setPoint(0, sf::Vector2f(screenX, baseY));
            tri.setPoint(1, sf::Vector2f(screenX + obs.width, baseY));
            tri.setPoint(2, sf::Vector2f(screenX + obs.width / 2.f, baseY - obs.height));
            tri.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
            window.draw(tri);
        }
    }

    player.render(window, 150.f);

    if (fontLoaded) {
        float progress = std::min(100.f, (cameraX / level.getLength()) * 100.f);
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << progress << "%";

        sf::Text progressText(font, ss.str(), 20);
        progressText.setFillColor(sf::Color::White);
        progressText.setPosition(sf::Vector2f(WIDTH - 70.f, 20.f));
        window.draw(progressText);

        if (!started && !dead && !won){
            sf::Text startText(font, "Click or press SPACE to start", 24);
            startText.setFillColor(sf::Color::White);
            startText.setPosition(sf::Vector2f(WIDTH / 2.f - 200.f, HEIGHT / 2.f - 60.f));
            window.draw(startText);
        }

        if (dead) {
            sf::Text deadText(font, "You died - click for level select", 32);
            deadText.setFillColor(sf::Color(0xff, 0x5e, 0x5e));
            deadText.setPosition(sf::Vector2f(WIDTH / 2.f - 230.f, HEIGHT / 2.f - 20.f));
            window.draw(deadText);
        }

        if (won) {
            sf::Text winText(font, "Level complete! - click for level select", 32);
            winText.setFillColor(sf::Color(0x7c, 0xf2, 0x7c));
            winText.setPosition(sf::Vector2f(WIDTH / 2.f - 280.f, HEIGHT / 2.f - 20.f));
            window.draw(winText);
        }
    }
}

void Game::render(){
    window.clear(sf::Color(0x1b, 0x1b, 0x2b));

    if (state == GameState::MainMenu){
        renderMainMenu();
    }else if (state == GameState::LevelSelect){
        renderLevelSelect();
    }else if (state == GameState::Playing){
        renderPlaying();
    }
    window.display();
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleInput();
        update(dt);
        render();
    }
}