#include "Game.h"
#include "CollisionUtils.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cctype>

Game::Game()
    : window(sf::VideoMode(sf::Vector2u((unsigned int)WIDTH, (unsigned int)HEIGHT)), "OrbDash"),
      level(Level::testLevel())
{
    window.setFramerateLimit(144);
    fontLoaded = font.openFromFile("assets/font.ttf");

    levels = {
        { "Level 1 - Warmup", "assets/levels/level1.txt"},
        { "Level 2 - Rush", "assets/levels/level2.txt"},
        { "Level 3 - Ascent", "assets/levels/level3.txt"},
        { "Level 4 - Marathon", "assets/levels/level4.txt"}
    };

    loadCustomLevels();

    bestTimes.assign(levels.size(), -1.f);
    loadBestTimes();
}

void Game::startLevel(int index, const std::string& path){
    currentLevelIndex = index;
    level = Level::loadFromFile(path);
    player.reset();
    cameraX = 0.f;
    dead = false;
    won = false;
    started = false;
    elapsedTime = 0.f;
    boosting = false;
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

Game::LevelGridLayout Game::computeLevelGridLayout() const {
    LevelGridLayout L;
    L.columns = 2;
    L.boxWidth = 320.f;
    L.boxHeight = 90.f;
    L.spacingX = 30.f;
    L.spacingY = 20.f;

    int rows = (int)((levels.size() + L.columns - 1) / L.columns);
    float totalWidth = L.columns * L.boxWidth + (L.columns - 1) * L.spacingX;
    float totalHeight = rows * L.boxHeight + std::max(0, rows - 1) * L.spacingY;
    L.startX = WIDTH / 2.f - totalWidth / 2.f;

    L.viewportTop = 140.f;
    L.viewportBottom = HEIGHT - 90.f;
    float viewportHeight = L.viewportBottom - L.viewportTop;

    L.maxScroll = std::max(0.f, totalHeight - viewportHeight);
    L.startY = (totalHeight <= viewportHeight) ? (L.viewportTop + (viewportHeight - totalHeight) / 2.f) : L.viewportTop;

    return L;
}

std::vector<sf::FloatRect> Game::levelButtonBounds() const {
    std::vector<sf::FloatRect> bounds;
    LevelGridLayout L = computeLevelGridLayout();

    for (size_t i = 0; i < levels.size(); i++){
        int col = i % L.columns;
        int row = i / L.columns;
        float x = L.startX + col * (L.boxWidth + L.spacingX);
        float y = L.startY + row * (L.boxHeight + L.spacingY) - levelSelectScrollY;
        bounds.push_back(sf::FloatRect(sf::Vector2f(x, y), sf::Vector2f(L.boxWidth, L.boxHeight)));      
    }
    return bounds;
}

void Game::handleMainMenuClick(sf::Vector2f mousePos){
    if (playButtonBounds().contains(mousePos)){
        state = GameState::LevelSelect;
    }
}

void Game::handleLevelSelectClick(sf::Vector2f mousePos){
    if (createLevelButtonBounds().contains(mousePos)){
        editorObstacles.clear();
        editorCameraX = 0.f;
        editorTool = EditorTool::Spike;
        editorNaming = false;
        state = GameState::Editor;
        return;
    }

    LevelGridLayout L = computeLevelGridLayout();
    auto bounds = levelButtonBounds();
    for (size_t i = 0; i < bounds.size(); i++){
        if (bounds[i].position.y + bounds[i].size.y <= L.viewportTop) continue;
        if (bounds[i].position.y >= L.viewportBottom) continue;
        if (bounds[i].contains(mousePos)){
            startLevel((int)i, levels[i].path);
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

            if (state == GameState::Editor){
                if (editorNaming){
                    if (keyPressed->code == sf::Keyboard::Key::Enter){
                        confirmEditorSave();
                    }else if (keyPressed->code == sf::Keyboard::Key::Escape){
                        editorNaming = false;
                    }else if (keyPressed->code == sf::Keyboard::Key::Backspace){
                        if (!editorNameInput.empty()) editorNameInput.pop_back();
                    }
                }else if(keyPressed->code == sf::Keyboard::Key::Escape){
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
            }else if (state == GameState::Editor){
                handleEditorMouseDown(mousePos);
            }
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()){
            if (state == GameState::Editor){
                sf::Vector2f mousePos((float)mouseReleased->position.x, (float)mouseReleased->position.y);
                handleEditorMouseUp(mousePos);
            }
        }

        if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()){
            if (state == GameState::LevelSelect){
                LevelGridLayout L = computeLevelGridLayout();
                levelSelectScrollY -= wheel->delta * 40.f;
                levelSelectScrollY = std::clamp(levelSelectScrollY, 0.f, L.maxScroll);
            }
        }

        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()){
            if (state == GameState::Editor && editorNaming){
                handleEditorTextEntered(textEntered->unicode);
            }
        }
    }
}

void Game::update(float dt){
    if (state != GameState::Playing) return;
    if (dead || !started) return;

    boosting = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

    float currentSpeed = SCROLL_SPEED * (boosting ? BOOST_MULTIPLIER : 1.f);
    cameraX += currentSpeed * dt;
    elapsedTime += dt;
    float playerWorldX = 150.f + cameraX;

    float floorY = Player::getGroundY();
    for (const auto& obs : level.getObstacles()){
        if (obs.type != ObstacleType::Platform) continue;

        float left = obs.worldX;
        float right = obs.worldX + obs.width;
        if (playerWorldX + Player::RADIUS < left || playerWorldX - Player::RADIUS > right) continue;

        float platformTop = Player::getGroundY() - obs.height;
        if (player.getY() - Player::RADIUS <= platformTop + 1.f){
            if (platformTop < floorY){
                floorY = platformTop;
            }
        }
    }

    player.update(dt, floorY);

    for (const auto& obs : level.getObstacles()){
        if (obs.type == ObstacleType::Platform) continue;

        bool hit;
        if (obs.type == ObstacleType::Block){
            float top = Player::getGroundY() - obs.height;
            hit = CollisionUtils::circleRectCollides(playerWorldX, player.getY(), Player::RADIUS,
                obs.worldX, top, obs.width, obs.height);
        }else {
            hit = CollisionUtils::circleTriangleCollides(playerWorldX, player.getY(), Player::RADIUS,
                obs.worldX, Player::getGroundY(), obs.width, obs.height);
        }
        if (hit) {
            dead = true;
            break;
        }
    }

    if (cameraX >= level.getLength()){
        won = true;
        started = false;

        if (currentLevelIndex >= 0){
            if (bestTimes[currentLevelIndex] < 0.f || elapsedTime < bestTimes[currentLevelIndex]){
                bestTimes[currentLevelIndex] = elapsedTime;
                saveBestTimes();
            }
            
        }
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

    LevelGridLayout L = computeLevelGridLayout();
    auto bounds = levelButtonBounds();

    sf::View clipView(sf::FloatRect(sf::Vector2f(0.f, L.viewportTop), sf::Vector2f(WIDTH, L.viewportBottom - L.viewportTop)));
    clipView.setViewport(sf::FloatRect(sf::Vector2f(0.f, L.viewportTop / HEIGHT), sf::Vector2f(1.f, (L.viewportBottom - L.viewportTop) / HEIGHT)));
    window.setView(clipView);

    for (size_t i = 0; i < bounds.size(); i++){
        if (bounds[i].position.y + bounds[i].size.y <= L.viewportTop) continue;
        if (bounds[i].position.y >= L.viewportBottom) continue;

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

        sf::Text timeText(font, "Best: " + formatTime(bestTimes[i]), 16);
        timeText.setFillColor(sf::Color(0xcc, 0xcc, 0xcc));
        timeText.setPosition(sf::Vector2f(bounds[i].position.x + 15.f, bounds[i].position.y + 58.f));
        window.draw(timeText);
    }

    window.setView(window.getDefaultView());

    if (L.maxScroll > 0.f){
        sf::Text scrollHint(font, "Scroll for more levels", 14);
        scrollHint.setFillColor(sf::Color(0xaa, 0xaa, 0xaa));
        scrollHint.setPosition(sf::Vector2f(WIDTH / 2.f - 80.f, L.viewportBottom + 4.f));
        window.draw(scrollHint);
    }

    sf::FloatRect createB = createLevelButtonBounds();
    sf::RectangleShape createBtn(createB.size);
    createBtn.setPosition(createB.position);
    createBtn.setFillColor(sf::Color(0x5e, 0xc4, 0xff));
    window.draw(createBtn);

    sf::Text createLabel(font, "+ CREATE LEVEL", 18);
    createLabel.setFillColor(sf::Color(0x1b, 0x1b, 0x2b));
    createLabel.setPosition(sf::Vector2f(createB.position.x + 25.f, createB.position.y + 13.f));
    window.draw(createLabel);
}

void Game::renderPlaying(){
    sf::RectangleShape ground(sf::Vector2f(WIDTH, HEIGHT - Player::getGroundY()));
    ground.setPosition(sf::Vector2f(0.f, Player::getGroundY()));
    ground.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(ground);

    for (const auto& obs : level.getObstacles()){
        float screenX = obs.worldX - cameraX;
        if (screenX < -100.f || screenX > WIDTH + 100.f) continue;
        drawObstacleShape(obs, screenX);
    }

    player.render(window, 150.f);

    if (fontLoaded) {
        if (boosting && started && !dead && !won){
            sf::Text boostText(font, "BOOST", 20);
            boostText.setFillColor(sf::Color(0xff, 0xd0, 0x4d));
            boostText.setPosition(sf::Vector2f(20.f, 20.f));
            window.draw(boostText);
        }
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

            std::string bestStr = (currentLevelIndex >= 0) ? formatTime(bestTimes[currentLevelIndex]) : "--:--";
            sf::Text timeText(font, "Time: " + formatTime(elapsedTime) + "  Best: " + bestStr, 22);
            timeText.setFillColor(sf::Color::White);
            timeText.setPosition(sf::Vector2f(WIDTH / 2.f - 150.f, HEIGHT / 2.f + 30.f));
            window.draw(timeText);
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
    }else if (state == GameState::Editor){
        renderEditor();
    }
    window.display();
}

void Game::run() {
    sf::Clock clock;
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.05f) dt = 0.05f;

        handleInput();
        if (state == GameState::Editor){
            updateEditor(dt);
        }else{
            update(dt);
        }
        render();
    }
}

void Game::loadBestTimes(){
    std::ifstream in("besttimes.txt");
    if(!in.is_open()) return;

    for (size_t i = 0; i < bestTimes.size() && in; i++){
        float t;
        if (in >> t){
            bestTimes[i] = t;
        }
    }
}

void Game::saveBestTimes(){
    std::ofstream out("besttimes.txt");
    for (float t : bestTimes){
        out << t << "\n";
    }
}

std::string Game::formatTime(float seconds) const {
    if (seconds < 0.f) return "--:--";
    int mins = (int)seconds / 60;
    float secs = seconds - mins * 60;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%05.2f", mins, secs);
    return std::string(buf);
}

void Game::loadCustomLevels(){
    std::ifstream in("assets/levels/custom_levels.txt");
    if (!in.is_open()) return;

    std::string line;
    int count = 0;
    while (std::getline(in, line)){
        if (line.empty()) continue;
        size_t sep = line.find('|');
        if (sep == std::string::npos) continue;

        std::string name = line.substr(0, sep);
        std::string path = line.substr(sep + 1);
        levels.push_back({name, path});
        count++;
    }
    nextCustomLevelNumber = count + 1;
}

sf::FloatRect Game::createLevelButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(WIDTH / 2.f - 110.f, HEIGHT - 65.f), sf::Vector2f(220.f, 45.f));
}

std::vector<sf::FloatRect> Game::editorToolButtonBounds() const {
    std::vector<sf::FloatRect> bounds;
    float w = 100.f, h = 44.f, gap = 10.f, x = 20.f, y = 13.f;
    for (int i = 0; i < 4; i++){
        bounds.push_back(sf::FloatRect(sf::Vector2f(x + i * (w + gap), y), sf::Vector2f(w, h)));
    }
    return bounds;
}

sf::FloatRect Game::editorClearButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(500.f, 13.f), sf::Vector2f(90.f, 44.f));
}

sf::FloatRect Game::editorSaveButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(600.f, 13.f), sf::Vector2f(90.f, 44.f));
}

sf::FloatRect Game::editorBackButtonBounds() const {
    return sf::FloatRect(sf::Vector2f(780.f, 13.f), sf::Vector2f(100.f, 44.f));
}

void Game::drawObstacleShape(const Obstacle& obs, float screenX){
    if (obs.type == ObstacleType::Block){
        sf::RectangleShape rect(sf::Vector2f(obs.width, obs.height));
        rect.setPosition(sf::Vector2f(screenX, Player::getGroundY() - obs.height));
        rect.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
        window.draw(rect);
    }else if (obs.type == ObstacleType::Spike){
        sf::ConvexShape tri(3);
        float baseY = Player::getGroundY();
        tri.setPoint(0, sf::Vector2f(screenX, baseY));
        tri.setPoint(1, sf::Vector2f(screenX + obs.width, baseY));
        tri.setPoint(2, sf::Vector2f(screenX + obs.width / 2.f, baseY - obs.height));
        tri.setFillColor(sf::Color(0xe8, 0xa3, 0x3d));
        window.draw(tri);
    }else if (obs.type == ObstacleType::Platform){
        float topY = Player::getGroundY() - obs.height;
        sf::RectangleShape plat(sf::Vector2f(obs.width, 14.f));
        plat.setPosition(sf::Vector2f(screenX, topY));
        plat.setFillColor(sf::Color(0x7c, 0xf2, 0x7c));
        window.draw(plat);
    }
}

void Game::eraseObstacleNear(float worldX){
    for (size_t i = 0; i < editorObstacles.size(); i++){
        const auto& obs = editorObstacles[i];
        float left = obs.worldX - 10.f;
        float right = obs.worldX + obs.width + 10.f;
        if (worldX >= left && worldX <= right){
            editorObstacles.erase(editorObstacles.begin() + i);
            return;
        }
    }
}

void Game::handleEditorMouseDown(sf::Vector2f mousePos){
    if (editorNaming) return;

    auto toolBounds = editorToolButtonBounds();
    for (size_t i = 0; i < toolBounds.size(); i++){
        if (toolBounds[i].contains(mousePos)){
            editorTool = (EditorTool)i;
            return;
        }
    }
    if (editorSaveButtonBounds().contains(mousePos)){
        editorNaming = true;
        editorNameInput.clear();
        return;
    }
    if (editorClearButtonBounds().contains(mousePos)){
        editorObstacles.clear();
        return;
    }
    if (editorBackButtonBounds().contains(mousePos)){
        state = GameState::LevelSelect;
        return;
    }

    if (mousePos.y < EDITOR_TOOLBAR_HEIGHT) return;

    float worldX = mousePos.x + editorCameraX;

    if (editorTool == EditorTool::Spike){
        editorObstacles.push_back({ObstacleType::Spike, worldX, 40.f, 40.f});
    }else if (editorTool == EditorTool::Eraser){
        eraseObstacleNear(worldX);
    }else{
        editorDragging = true;
        editorDragStartWorldX = worldX;
    }
}

void Game::handleEditorMouseUp(sf::Vector2f mousePos){
    if (!editorDragging) return;
    editorDragging = false;

    float worldX = mousePos.x + editorCameraX;
    float left = std::min(editorDragStartWorldX, worldX);

    if (editorTool == EditorTool::Block){
        float size = std::max(std::abs(worldX - editorDragStartWorldX), 20.f);
        editorObstacles.push_back({ObstacleType::Block, left, size, size});
    }else if (editorTool == EditorTool::Platform){
        float width = std::max(std::abs(worldX - editorDragStartWorldX), 20.f);
        float height = Player::getGroundY() - mousePos.y;
        height = std::clamp(height, 30.f, 220.f);
        editorObstacles.push_back({ObstacleType::Platform, left, width, height});
    }
}

void Game::handleEditorTextEntered(char32_t unicode){
    if (unicode < 32 || unicode > 126) return;
    if (editorNameInput.size() >= 24) return;
    editorNameInput += (char)unicode;
}

void Game::confirmEditorSave(){
    if (editorNameInput.empty()) return;
    editorNaming = false;

    std::string displayName = editorNameInput;
    std::string slug;
    for (char c : displayName){
        if (std::isalnum((unsigned char)c)) slug += (char)std::tolower((unsigned char)c);
        else if (c == ' ') slug += '_';
    }
    if (slug.empty()) slug = "level";

    std::string path = "assets/levels/custom_" + slug + "_" + std::to_string(nextCustomLevelNumber) + ".txt";
    nextCustomLevelNumber++;

    float length = 800.f;
    for (const auto& obs : editorObstacles){
        length = std::max(length, obs.worldX + obs.width + 300.f);
    }

    std::ofstream out(path);
    out << "LENGTH " << length << "\n";
    for (const auto& obs : editorObstacles){
        if (obs.type == ObstacleType::Spike){
            out << "SPIKE " << obs.worldX << "\n";
        }else if (obs.type == ObstacleType::Block){
            out << "BLOCK " << obs.worldX << " " << obs.width << "\n";
        }else if (obs.type == ObstacleType::Platform){
            out << "PLATFORM " << obs.worldX << " " << obs.width << " " << obs.height << "\n";
        }
    }
    out.close();

    std::ofstream manifest("assets/levels/custom_levels.txt", std::ios::app);
    manifest << displayName << "|" << path << "\n";
    manifest.close();

    levels.push_back({displayName, path});
    bestTimes.push_back(-1.f);

    editorObstacles.clear();
    editorCameraX = 0.f;
    editorNameInput.clear();
    state = GameState::LevelSelect;
}

void Game::updateEditor(float dt){
    if (editorNaming) return;

    float panSpeed = 700.f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)){
        editorCameraX = std::max(0.f, editorCameraX - panSpeed * dt);
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)){
        editorCameraX += panSpeed * dt;
    }
}

void Game::renderEditor(){
    if (!fontLoaded) return;

    sf::RectangleShape ground(sf::Vector2f(WIDTH, HEIGHT - Player::getGroundY()));
    ground.setPosition(sf::Vector2f(0.f, Player::getGroundY()));
    ground.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(ground);

    for (const auto& obs : editorObstacles){
        float screenX = obs.worldX - editorCameraX;
        if (screenX < -150.f || screenX > WIDTH + 150.f) continue;
        drawObstacleShape(obs, screenX);
    }

    if (editorDragging){
        sf::Vector2i mp = sf::Mouse::getPosition(window);
        float worldX = (float)mp.x + editorCameraX;
        float left = std::min(editorDragStartWorldX, worldX);
        float screenX = left - editorCameraX;

        if (editorTool == EditorTool::Block){
            float size = std::max(std::abs(worldX - editorDragStartWorldX), 20.f);
            sf::RectangleShape rect(sf::Vector2f(size, size));
            rect.setPosition(sf::Vector2f(screenX, Player::getGroundY() - size));
            rect.setFillColor(sf::Color(0xe8, 0xa3, 0x3d, 140));
            window.draw(rect);
        }else if (editorTool == EditorTool::Platform){
            float width = std::max(std::abs(worldX - editorDragStartWorldX), 20.f);
            float height = std::clamp(Player::getGroundY() - (float)mp.y, 30.f, 220.f);
            sf::RectangleShape plat(sf::Vector2f(width, 14.f));
            plat.setPosition(sf::Vector2f(screenX, Player::getGroundY() - height));
            plat.setFillColor(sf::Color(0x7c, 0xf2, 0x7c, 140));
            window.draw(plat);
        }
    }

    sf::RectangleShape bar(sf::Vector2f(WIDTH, EDITOR_TOOLBAR_HEIGHT));
    bar.setPosition(sf::Vector2f(0.f, 0.f));
    bar.setFillColor(sf::Color(0x14, 0x14, 0x22));
    window.draw(bar);

    auto toolBounds = editorToolButtonBounds();
    const char* toolNames[4] = {"SPIKE", "BLOCK", "PLATFORM", "ERASE"};
    for (int i = 0; i < 4; i++){
        sf::RectangleShape btn(toolBounds[i].size);
        btn.setPosition(toolBounds[i].position);
        bool selected = ((int)editorTool == i);
        btn.setFillColor(selected ? sf::Color(0x5e, 0xc4, 0xff) : sf::Color(0x2c, 0x2c, 0x44));
        btn.setOutlineColor(sf::Color(0xe8, 0xa3, 0x3d));
        btn.setOutlineThickness(1.f);
        window.draw(btn);

        sf::Text label(font, toolNames[i], 14);
        label.setFillColor(selected ? sf::Color(0x1b, 0x1b, 0x2b) : sf::Color::White);
        label.setPosition(sf::Vector2f(toolBounds[i].position.x + 10.f, toolBounds[i].position.y + 13.f));
        window.draw(label);
    }

    sf::FloatRect clearB = editorClearButtonBounds();
    sf::RectangleShape clearBtn(clearB.size);
    clearBtn.setPosition(clearB.position);
    clearBtn.setFillColor(sf::Color(0x44, 0x2c, 0x2c));
    window.draw(clearBtn);
    sf::Text clearLabel(font, "CLEAR", 14);
    clearLabel.setFillColor(sf::Color::White);
    clearLabel.setPosition(sf::Vector2f(clearB.position.x + 12.f, clearB.position.y + 13.f));
    window.draw(clearLabel);

    sf::FloatRect saveB = editorSaveButtonBounds();
    sf::RectangleShape saveBtn(saveB.size);
    saveBtn.setPosition(saveB.position);
    saveBtn.setFillColor(sf::Color(0x2c, 0x44, 0x2c));
    window.draw(saveBtn);
    sf::Text saveLabel(font, "SAVE", 14);
    saveLabel.setFillColor(sf::Color::White);
    saveLabel.setPosition(sf::Vector2f(saveB.position.x + 20.f, saveB.position.y + 13.f));
    window.draw(saveLabel);

    sf::FloatRect backB = editorBackButtonBounds();
    sf::RectangleShape backBtn(backB.size);
    backBtn.setPosition(backB.position);
    backBtn.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
    window.draw(backBtn);
    sf::Text backLabel(font, "BACK", 14);
    backLabel.setFillColor(sf::Color::White);
    backLabel.setPosition(sf::Vector2f(backB.position.x + 25.f, backB.position.y + 13.f));
    window.draw(backLabel);

    sf::Text hint(font, "A/D or Arrows to scroll  |  Click = place  |  Drag = size (Block/Platform)", 14);
    hint.setFillColor(sf::Color(0xaa, 0xaa, 0xaa));
    hint.setPosition(sf::Vector2(20.f, HEIGHT - 30.f));
    window.draw(hint);

    if (editorNaming){
        sf::RectangleShape overlay(sf::Vector2f(WIDTH, HEIGHT));
        overlay.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(overlay);

        sf::RectangleShape box(sf::Vector2f(400.f, 140.f));
        box.setPosition(sf::Vector2f(WIDTH / 2.f - 200.f, HEIGHT / 2.f - 70.f));
        box.setFillColor(sf::Color(0x2c, 0x2c, 0x44));
        box.setOutlineColor(sf::Color(0xe8, 0xa3, 0x3d));
        box.setOutlineThickness(2.f);
        window.draw(box);

        sf::Text prompt(font, "Name your level (Enter to save):", 16);
        prompt.setFillColor(sf::Color::White);
        prompt.setPosition(sf::Vector2f(WIDTH / 2.f - 180.f, HEIGHT / 2.f - 50.f));
        window.draw(prompt);

        sf::Text nameText(font, editorNameInput + "_", 22);
        nameText.setFillColor(sf::Color(0x7c, 0xf2, 0x7c));
        nameText.setPosition(sf::Vector2f(WIDTH / 2.f - 180.f, HEIGHT / 2.f - 10.f));
        window.draw(nameText);
    }
}