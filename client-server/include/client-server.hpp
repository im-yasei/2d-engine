#pragma once
#include "engine.hpp"
#include <SFML/System/Vector2.hpp>
#include <X11/X.h>
#include <arpa/inet.h>
#include <atomic>
#include <cstdint>
#include <fcntl.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

extern std::atomic<bool> clientRunning;

uint32_t float_to_network(float value);

float network_to_float(uint32_t value);

void server_send_broadcast(int sockfd, std::vector<Planet> *planets,
                           std::mutex *planets_mutex, int port,
                           const std::string &ip, float *G, float *timeStep);

void client_receive(int sockfd, std::vector<Planet> *planets,
                    std::mutex *planets_mutex,
                    std::vector<std::vector<sf::Vertex>> *trajectories);
