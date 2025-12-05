#include "engine.hpp"
#include "tests.hpp"
#include "physics.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <vector>


TEST(PhysicsTest, PlanetInitialization) {
    Planet p(10.0f, 5.0f);
    
    EXPECT_FLOAT_EQ(p.getRadius(), 10.0f);
    EXPECT_FLOAT_EQ(p.getMass(), 5.0f);
    
    EXPECT_FLOAT_EQ(p.getPosition().x, 0.0f);
    EXPECT_FLOAT_EQ(p.getPosition().y, 0.0f);
    
    EXPECT_FLOAT_EQ(p.getVelocity().x, 0.0f);
    EXPECT_FLOAT_EQ(p.getVelocity().y, 0.0f);
}

TEST(PhysicsTest, PlanetSettersAndGetters) {
    Planet p(5.0f, 2.0f);
    
    p.setPosition(sf::Vector2f(100.0f, 50.0f));
    EXPECT_FLOAT_EQ(p.getPosition().x, 100.0f);
    EXPECT_FLOAT_EQ(p.getPosition().y, 50.0f);
    
    p.setVelocity(sf::Vector2f(2.0f, -1.0f));
    EXPECT_FLOAT_EQ(p.getVelocity().x, 2.0f);
    EXPECT_FLOAT_EQ(p.getVelocity().y, -1.0f);
    
    p.move(sf::Vector2f(10.0f, 5.0f));
    EXPECT_FLOAT_EQ(p.getPosition().x, 110.0f);
    EXPECT_FLOAT_EQ(p.getPosition().y, 55.0f);
}

TEST(PhysicsTest, CollisionDetection) {
    Planet p1(10.0f, 5.0f);
    Planet p2(10.0f, 5.0f);
    
    // Planets touch each other (not collide)
    p1.setPosition(sf::Vector2f(0.0f, 0.0f));
    p2.setPosition(sf::Vector2f(20.0f, 0.0f));
    
    EXPECT_FALSE(p1.isColliding(p2));
    
    // Planets collide
    p2.setPosition(sf::Vector2f(15.0f, 0.0f));
    EXPECT_TRUE(p1.isColliding(p2));
    
    // Planets far from each other
    p2.setPosition(sf::Vector2f(25.0f, 0.0f));
    EXPECT_FALSE(p1.isColliding(p2));
}

TEST(PhysicsTest, GravityBetweenTwoPlanets) {
    Planet p1(10.0f, 100.0f);
    Planet p2(5.0f, 50.0f);
    
    p1.setPosition(sf::Vector2f(0.0f, 0.0f));
    p2.setPosition(sf::Vector2f(100.0f, 0.0f));
    
    std::vector<Planet> planets = {p1, p2};
    const float G = 100.0f;
    const float timeStep = 1.0f;
    
    sf::Vector2f initialV1 = planets[0].getVelocity();
    sf::Vector2f initialV2 = planets[1].getVelocity();
    
    applyGravity(planets, G, timeStep);
    
    // Check velocity changing
    EXPECT_NE(planets[0].getVelocity().x, initialV1.x);
    EXPECT_NE(planets[1].getVelocity().x, initialV2.x);
    
    // p1 should move towards p2 (right)
    EXPECT_GT(planets[0].getVelocity().x, initialV1.x);
    
    // p2 should move towards p1 (left)
    EXPECT_LT(planets[1].getVelocity().x, initialV2.x);
    
    // Vertical velocity should remain 0
    EXPECT_FLOAT_EQ(planets[0].getVelocity().y, 0.0f);
    EXPECT_FLOAT_EQ(planets[1].getVelocity().y, 0.0f);
}

TEST(PhysicsTest, GravityDistanceProtection) {
    Planet p1(10.0f, 100.0f);
    Planet p2(5.0f, 50.0f);
    
    // Planets very close to each other
    p1.setPosition(sf::Vector2f(0.0f, 0.0f));
    p2.setPosition(sf::Vector2f(0.5f, 0.5f));
    
    std::vector<Planet> planets = {p1, p2};
    const float G = 100.0f;
    const float timeStep = 1.0f;
    
    sf::Vector2f initialV1 = planets[0].getVelocity();
    sf::Vector2f initialV2 = planets[1].getVelocity();
    
    applyGravity(planets, G, timeStep);
    
    // If distance < 1, gravity shouldn't be applied
    EXPECT_FLOAT_EQ(planets[0].getVelocity().x, initialV1.x);
    EXPECT_FLOAT_EQ(planets[0].getVelocity().y, initialV1.y);
    EXPECT_FLOAT_EQ(planets[1].getVelocity().x, initialV2.x);
    EXPECT_FLOAT_EQ(planets[1].getVelocity().y, initialV2.y);
}

TEST(PhysicsTest, ElasticCollision) {
    Planet p1(10.0f, 10.0f);
    Planet p2(10.0f, 10.0f);
    
    p1.setPosition(sf::Vector2f(0.0f, 0.0f));
    p2.setPosition(sf::Vector2f(15.0f, 0.0f)); //Collission
    
    p1.setVelocity(sf::Vector2f(5.0f, 0.0f));
    p2.setVelocity(sf::Vector2f(-5.0f, 0.0f));
    
    std::vector<Planet> planets = {p1, p2};
    
    sf::Vector2f initialMomentum = planets[0].getVelocity() * planets[0].getMass() +
                                  planets[1].getVelocity() * planets[1].getMass();
    
    applyCollision(planets);
    
    sf::Vector2f finalMomentum = planets[0].getVelocity() * planets[0].getMass() +
                                planets[1].getVelocity() * planets[1].getMass();
    // Momentums should remain the same
    EXPECT_NEAR(initialMomentum.x, finalMomentum.x, EPSILON);
    EXPECT_NEAR(initialMomentum.y, finalMomentum.y, EPSILON);
    
    // Velocities should have changed
    EXPECT_NE(planets[0].getVelocity().x, 5.0f);
    EXPECT_NE(planets[1].getVelocity().x, -5.0f);
}

TEST(PhysicsTest, MultiplePlanetsGravity) {
    std::vector<Planet> planets;
    
    // Placing planets on equilateral triangle
    Planet p1(5.0f, 10.0f);
    p1.setPosition(sf::Vector2f(0.0f, 0.0f));
    p1.setVelocity(sf::Vector2f(0.0f, 0.0f));
    
    Planet p2(5.0f, 10.0f);
    p2.setPosition(sf::Vector2f(100.0f, 0.0f));
    p2.setVelocity(sf::Vector2f(0.0f, 0.0f));
    
    Planet p3(5.0f, 10.0f);
    p3.setPosition(sf::Vector2f(50.0f, 86.6f));
    p3.setVelocity(sf::Vector2f(0.0f, 0.0f));
    
    planets.push_back(p1);
    planets.push_back(p2);
    planets.push_back(p3);
    
    const float G = 100.0f;
    const float timeStep = 1.0f;
    
    std::vector<sf::Vector2f> initialPositions;
    std::vector<sf::Vector2f> initialVelocities;
    for (const auto& p : planets) {
        initialPositions.push_back(p.getPosition());
        initialVelocities.push_back(p.getVelocity());
    }
    
    for (int i = 0; i < 5; i++) {
        applyGravity(planets, G, timeStep);
    }
    
    sf::Vector2f initialCenterOfMass(0.0f, 0.0f);
    sf::Vector2f finalCenterOfMass(0.0f, 0.0f);
    float totalMass = 0.0f;
    
    for (size_t i = 0; i < planets.size(); i++) {
        initialCenterOfMass += initialPositions[i] * planets[i].getMass();
        finalCenterOfMass += planets[i].getPosition() * planets[i].getMass();
        totalMass += planets[i].getMass();
    }
    
    initialCenterOfMass /= totalMass;
    finalCenterOfMass /= totalMass;
    
    // Center of mass should remain the same
    EXPECT_NEAR(initialCenterOfMass.x, finalCenterOfMass.x, EPSILON);
    EXPECT_NEAR(initialCenterOfMass.y, finalCenterOfMass.y, EPSILON);
    
    // Planets should have started moving towards center of mass
    for (size_t i = 0; i < planets.size(); i++) {
        EXPECT_FALSE(planets[i].getVelocity() == sf::Vector2f(0.0f, 0.0f))
            << "Planet " << i << " should have non-zero velocity after gravity";
        
        if (planets[i].getVelocity() != sf::Vector2f(0.0f, 0.0f)) {
            EXPECT_NE(planets[i].getPosition().x, initialPositions[i].x);
            EXPECT_NE(planets[i].getPosition().y, initialPositions[i].y);
        }
    }
    
    sf::Vector2f totalMomentum(0.0f, 0.0f);
    for (const auto& planet : planets) {
        totalMomentum += planet.getVelocity() * planet.getMass();
    }
    
    // Momenutm should remain the same as in the beginning (0)
    EXPECT_NEAR(totalMomentum.x, 0.0f, EPSILON * 10);
    EXPECT_NEAR(totalMomentum.y, 0.0f, EPSILON * 10);
}