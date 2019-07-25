#pragma once

#include "glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/transform.hpp"
#include "glm/gtx/string_cast.hpp"

class Camera
{
public:
    Camera(float size);
    virtual ~Camera();

    void reset();
    glm::mat4 rotate(int32_t x, int32_t y, int32_t xrel, int32_t yrel, int32_t width, int32_t height);
    glm::mat4 zoom(int32_t y);
    glm::mat4 translate(int32_t xrel, int32_t yrel);
    glm::mat4 getModelView();
    glm::mat4 getPerspective();

private:
    glm::vec2 scale2snorm(int32_t x, int32_t y, int32_t w, int32_t h);
    glm::vec3 project2Spehere(glm::vec2 xy);

private:
    float m_size;
    glm::vec3 m_eye;
    glm::vec3 m_origin;
    glm::vec3 m_up;

    glm::mat4 m_model;
    glm::mat4 m_view;
    glm::mat4 m_perspective;
};

