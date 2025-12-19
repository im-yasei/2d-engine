#pragma once
#include "engine.hpp"
#include <SFML/System/Vector2.hpp>
#include <X11/X.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <imgui-SFML.h>
#include <imgui.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

void applyGravity(std::vector<Planet> &planets, float G, float timeStep);
void applyCollision(std::vector<Planet> &planets);
