#include "json/json.h"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include "engine.hpp"

void applyGravity(std::vector<Planet> &planets, float G, float timeStep);
void applyCollision(std::vector<Planet> &planets);