#include <cmath>

const float EPSILON = 0.01f;

inline bool compareVectors(sf::Vector2f v1, sf::Vector2f v2, float epsilon = EPSILON) {
    return std::fabs(v1.x - v2.x) < epsilon && 
           std::fabs(v1.y - v2.y) < epsilon;
}
