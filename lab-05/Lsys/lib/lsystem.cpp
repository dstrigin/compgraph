#include "lsystem.h"
#include <fstream>
#include <iostream>

LSystem::LSystem() : angle(90.0f), initialAngle(0.0f) {}

bool LSystem::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    std::getline(file, axiom);
    file >> angle;
    file >> initialAngle;

    rules.clear();
    std::string line;
    std::getline(file, line); 
    while (std::getline(file, line)) {
        if (line.length() > 2 && line[1] == '=') {
            rules[line[0]] = line.substr(2);
        }
    }

    file.close();
    return true;
}

std::string LSystem::generate(int iterations) {
    std::string current = axiom;
    for (int i = 0; i < iterations; ++i) {
        std::string next = "";
        for (char c : current) {
            if (rules.count(c)) {
                next += rules[c];
            } else {
                next += c;
            }
        }
        current = next;
    }
    return current;
}

const std::string& LSystem::getAxiom() const { return axiom; }
const std::map<char, std::string>& LSystem::getRules() const { return rules; }
float LSystem::getAngle() const { return angle; }
float LSystem::getInitialAngle() const { return initialAngle; }