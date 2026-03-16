//
// Created by ruut on 3/16/26.
//

#ifndef FLUIDSIMSPH_TIME_H
#define FLUIDSIMSPH_TIME_H

#include <chrono>

class Time {
public:
    double getTotalTime() const;
    double getDeltaTime() const;

    void update();
private:
    double totalTime = 0.0;
    double deltaTime = 0.0;

    std::chrono::high_resolution_clock::time_point lastTime = std::chrono::high_resolution_clock::now();
};

inline Time Time;

#endif //FLUIDSIMSPH_TIME_H