//
// Created by ruut on 3/16/26.
//

#include "core/Time.h"

double Time::getTotalTime() const { return totalTime; }
double Time::getDeltaTime() const { return deltaTime; }

void Time::update() {
    auto now = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<double>(now - lastTime).count();
    totalTime += deltaTime;
    lastTime = now;
}
