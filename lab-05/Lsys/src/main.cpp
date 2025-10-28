#include <SFML/Graphics.hpp>
#include "../lib/lsystem.h"
#include <stack>
#include <iostream>
#include <cmath>
#include <random>
#include <functional>
#include <random>
#include <algorithm>

const float PI = 3.14159265359f;

struct TurtleState {
    sf::Vector2f position;
    float angle;
    float thickness;
    sf::Color color;
    int depth;
    float line_length;
};

void drawThickLine(sf::RenderWindow& window, sf::Vector2f p1, sf::Vector2f p2, float thickness, sf::Color color) {
    if (p1 == p2) return;
    
    sf::Vector2f direction = p2 - p1;
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    
    sf::RectangleShape line(sf::Vector2f(length, std::max(1.0f, thickness)));
    line.setOrigin(0, thickness / 2.f);
    line.setPosition(p1);
    line.setFillColor(color);
    line.setRotation(atan2(direction.y, direction.x) * 180 / PI);
    window.draw(line);
}

sf::Color lerpColor(const sf::Color& a, const sf::Color& b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t)
    );
}

int main() {
    const unsigned int WINDOW_WIDTH = 1200;
    const unsigned int WINDOW_HEIGHT = 1000;
    
    // --- ВЫБЕРИТЕ ФАЙЛ ДЛЯ ЗАПУСКА ---
    const std::string FILENAME = "rules/tree"; int ITERATIONS = 12;
    //const std::string FILENAME = "rules/koch"; int ITERATIONS = 5;
    //const std::string FILENAME = "rules/dragon"; int ITERATIONS = 15;


    const float LINE_LENGTH = 30.f; 
    const bool isTree = (FILENAME == "rules/tree");
    const float INITIAL_THICKNESS = isTree ? 15.0f : 5.f;

    // --- ЗАГРУЗКА L-СИСТЕМЫ ---
    LSystem lsys;
    if (!lsys.loadFromFile(FILENAME)) {
        return -1;
    }
    std::string generatedString = lsys.generate(ITERATIONS);
    float baseAngle = lsys.getAngle();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> angle_dis(-15.0f, 15.0f);
    std::uniform_real_distribution<> line_dis(0.5f, 1.f);

    // --- 1-й ПРОХОД: ВЫЧИСЛЕНИЕ ГРАНИЦ ДЛЯ МАСШТАБИРОВАНИЯ ---
    sf::Vector2f minBounds(0, 0), maxBounds(0, 0);
    {
        TurtleState currentState = {{0, 0}, lsys.getInitialAngle(), INITIAL_THICKNESS, sf::Color::White, 0, LINE_LENGTH};
        std::stack<TurtleState> stateStack;
        std::string drawChars = "FG";

        for (char c : generatedString) {
            if (drawChars.find(c) != std::string::npos) { 
                float angleRad = currentState.angle * PI / 180.f;
                currentState.position.x += currentState.line_length * cos(angleRad);
                currentState.position.y += currentState.line_length * sin(angleRad);
                
                minBounds.x = std::min(minBounds.x, currentState.position.x);
                minBounds.y = std::min(minBounds.y, currentState.position.y);
                maxBounds.x = std::max(maxBounds.x, currentState.position.x);
                maxBounds.y = std::max(maxBounds.y, currentState.position.y);
            } else if (c == '+') {
                currentState.angle += baseAngle;
            } else if (c == '-') {
                currentState.angle -= baseAngle;
            } else if (c == '[') {
                stateStack.push(currentState);
                if (isTree) {
                    currentState.thickness *= 0.8f;
                    currentState.line_length *= 0.8f;
                }
            } else if (c == ']') {
                if (!stateStack.empty()) {
                    currentState = stateStack.top();
                    stateStack.pop();
                }
            }
        }
    }

    sf::Vector2f fractalSize = maxBounds - minBounds;
    float scaleX = (fractalSize.x == 0) ? 1.0f : (WINDOW_WIDTH * 0.9f) / fractalSize.x;
    float scaleY = (fractalSize.y == 0) ? 1.0f : (WINDOW_HEIGHT * 0.9f) / fractalSize.y;
    float scale = std::min(scaleX, scaleY);

    sf::Vector2f offset;
    offset.x = (WINDOW_WIDTH - (fractalSize.x * scale)) / 2.0f - minBounds.x * scale;
    
    if (isTree) {
        offset.y = (WINDOW_HEIGHT * 0.95f) - maxBounds.y * scale;
    } else {
        offset.y = (WINDOW_HEIGHT - (fractalSize.y * scale)) / 2.0f - minBounds.y * scale;
    }


    // --- 2-й ПРОХОД: ПОДГОТОВКА К ОТРИСОВКЕ ---
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "L-System Fractal Generator");
    window.setFramerateLimit(60);

    sf::Color brown(139, 69, 19);
    sf::Color green(0, 128, 0);

    struct DrawCall {
        float thickness;
        std::function<void(sf::RenderWindow&)> call;
    };
    std::vector<DrawCall> drawCalls;

    TurtleState currentState = {{0, 0}, lsys.getInitialAngle(), INITIAL_THICKNESS, isTree ? brown : sf::Color::White, 0, LINE_LENGTH};
    std::stack<TurtleState> stateStack;
    std::string drawChars = "FG";

    for (char c : generatedString) {
        if (drawChars.find(c) != std::string::npos) {
            sf::Vector2f startPos = currentState.position * scale + offset;

            float angleRad = (currentState.angle + (isTree ? angle_dis(gen) : 0.0f)) * PI / 180.f;
            currentState.position.x += currentState.line_length * cos(angleRad);
            currentState.position.y += currentState.line_length * sin(angleRad);

            sf::Vector2f endPos = currentState.position * scale + offset;
            float thickness = currentState.thickness;

            sf::Color color = color.White;

            float t = 0.0f;
            if (isTree) {
                t = static_cast<float>(currentState.depth) / static_cast<float>(ITERATIONS);
                t = std::clamp(t, 0.0f, 1.0f);
                color = lerpColor(brown, green, t);
            }
            
            drawCalls.push_back({thickness, [=](sf::RenderWindow& win) {
                drawThickLine(win, startPos, endPos, thickness, color);
            }});

        } else if (c == '+') {
            currentState.angle += baseAngle;
        } else if (c == '-') {
            currentState.angle -= baseAngle;
        } else if (c == '[') {
            stateStack.push(currentState);
            if (isTree) {
                currentState.thickness *= 0.8f;
                currentState.depth++;
                currentState.line_length *= 0.8f;
            }
        } else if (c == ']') {
            if (!stateStack.empty()) {
                currentState = stateStack.top();
                stateStack.pop();
            }
        }
    }

    std::sort(drawCalls.begin(), drawCalls.end(),
            [](const DrawCall& a, const DrawCall& b) {
                return a.thickness > b.thickness;
            });

    // --- ГЛАВНЫЙ ЦИКЛ ОТРИСОВКИ ---
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear(sf::Color::Black);

        for (const auto& dc : drawCalls) {
            dc.call(window);
        }

        window.display();
    }


    return 0;
}