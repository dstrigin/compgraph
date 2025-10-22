#ifndef LSYSTEM_H
#define LSYSTEM_H

#include <string>
#include <vector>
#include <map>

class LSystem {
public:
    LSystem();
    bool loadFromFile(const std::string& filename);
    std::string generate(int iterations);

    const std::string& getAxiom() const;
    const std::map<char, std::string>& getRules() const;
    float getAngle() const;
    float getInitialAngle() const;

private:
    std::string axiom;
    std::map<char, std::string> rules;
    float angle;
    float initialAngle;
};

#endif // LSYSTEM_H