#pragma once
#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

struct Light
{
    glm::vec3 pos;
    glm::vec3 color;
    Light(glm::vec3 pos, glm::vec3 color) {
        this->pos = pos;
        this->color = color;
    };
};



#endif