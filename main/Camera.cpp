#include "Camera.h"
#include <iostream>
#include <cmath>

Camera::Camera(float size) : m_size(size)
{
    reset();
}

Camera::~Camera()
{
}

void Camera::reset()
{
    m_eye = glm::vec3(0.0f, 0.0f, -2.0f);
    m_origin = glm::vec3(0.0f, 0.0f, 0.0f);
    m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    m_translate = glm::vec3(0.0f, 0.0f, 0.0f);

    m_view = glm::lookAt(m_eye, m_origin, m_up);
    m_model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    m_perspective = glm::perspective(glm::radians(90.0f), 1.0f /*m_vulkan.swapChain.extent.width / float(m_vulkan.swapChain.extent.height)*/, 0.1f, 10.0f);
}

glm::mat4 Camera::getModel()
{
    return m_model;
}

glm::mat4 Camera::getView()
{
    return m_view;
}

glm::mat4 Camera::getPerspective()
{
    return m_perspective;
}

glm::mat4 Camera::zoom(int32_t y)
{
    //glm::mat4 translate = glm::inverse(glm::lookAt(m_eye, m_origin, m_up)) * m_view;
    m_eye.z += y / 100.f;
    m_view = glm::lookAt(m_eye, m_origin, m_up);

    return m_view;
}

glm::mat4 Camera::translate(int32_t xrel, int32_t yrel)
{
    m_view = glm::translate(m_view, glm::vec3(-xrel/1000.f, yrel/1000.f, 0.0f));

    return m_view;
}

glm::mat4 Camera::rotate(int32_t x, int32_t y, int32_t xrel, int32_t yrel, int32_t width, int32_t height)
{
    //x, y is the new position;
    int32_t old_x = x - xrel;
    int32_t old_y = y - yrel;

    if (xrel == 0 && yrel == 0) return m_model;

    glm::vec2 old_pos = scale2snorm(old_x, old_y, width, height);
    glm::vec2 new_pos = scale2snorm(x, y, width, height);
    glm::vec3 p1 = project2Spehere(old_pos);
    glm::vec3 p2 = project2Spehere(new_pos);

    //std::cout << glm::to_string(p1)  << std::endl;
    //std::cout << glm::to_string(p2)  << std::endl;

    glm::vec3 r_axis = glm::cross(p1, p2);
    glm::mat3 camera2object = glm::inverse(glm::mat3(m_view) * glm::mat3(m_model));
    glm::vec3 r_axis_in_object = camera2object * r_axis;
    float r_angle = 180 * glm::length(p1 - p2);
    float angle = acos(glm::min(1.0f, glm::dot(p1, p2)));
    m_model = glm::rotate(m_model, r_angle, r_axis_in_object);

    return m_model;
}

glm::vec2 Camera::scale2snorm(int32_t x, int32_t y, int32_t w, int32_t h)
{
    return glm::vec2(x * 2.0 / w - 1.0f, y * 2.0f / h - 1.0f);
}

glm::vec3 Camera::project2Spehere(glm::vec2 xy)
{
    static const float sqrt2 = sqrt(2.0f);
    glm::vec3 result;

    float d = glm::length(xy);
    if (d < m_size * sqrt2 / 2.0f) {
        result.z = sqrt(m_size * m_size - d * d);
    } else {
        result.z = m_size * m_size / (2 * d);
    }
    result.x = xy.x;
    result.y = xy.y;

    return glm::normalize(result);
}
