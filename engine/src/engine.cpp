#include "engine.hpp"
#include <SFML/Graphics.hpp>

void Planet::setRadius(int newRadius) { radius = newRadius; }

int Planet::getRadius() const { return radius; }

void Planet::init() {
  shape.setRadius(static_cast<float>(radius));
  shape.setFillColor(sf::Color::Cyan);
  shape.setPosition(0, 0);
}

void Planet::draw(sf::RenderWindow &window) { window.draw(shape); }

void Planet::setPosition(float x, float y) { shape.setPosition(x, y); }
