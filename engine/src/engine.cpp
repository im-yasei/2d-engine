#include "engine.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>

void Planet::setRadius(float newRadius) { radius = newRadius; }

int Planet::getRadius() const { return radius; }

void Planet::init() {
  shape.setRadius(radius);
  shape.setFillColor(sf::Color::Cyan);
  shape.setPosition(0, 0);
}

void Planet::draw(sf::RenderWindow &window) { window.draw(shape); }

void Planet::setPosition(sf::Vector2f planetPosition) {
  shape.setPosition(planetPosition);
}

void Planet::move(sf::Vector2f changePosition) { shape.move(changePosition); }
