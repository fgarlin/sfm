#include "camera.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>


const float Z_NEAR = 0.1f;
const float Z_FAR  = 100.0f;


Camera::Camera() :
    _position(0.0f, 0.0f, 2.0f),
    _yaw(-90.0f),
    _pitch(0.0f)
{
    updateVectors();
}

Camera::Camera(glm::vec3 position, float yaw, float pitch) :
    _position(position),
    _yaw(yaw),
    _pitch(pitch)
{
    updateVectors();
}

void
Camera::move(MoveDirection dir, float dt)
{
    float v = _move_speed * dt;
    switch (dir) {
    case FORWARD:  _position += _front   * v; break;
    case BACKWARD: _position -= _front   * v; break;
    case RIGHT:    _position += _right   * v; break;
    case LEFT:     _position -= _right   * v; break;
    case UP:       _position += WORLD_UP * v; break;
    case DOWN:     _position -= WORLD_UP * v; break;
    }
}

void
Camera::look(float xoffset, float yoffset)
{
    xoffset *= _sensitivity;
    yoffset *= _sensitivity;
    _yaw   += xoffset;
    _pitch += yoffset;
    // Clamp so we can't look past up and down
    _pitch = std::max(-89.9f, std::min(_pitch, 89.9f));
    updateVectors();
}

glm::mat4
Camera::getViewMatrix() const
{
    return glm::lookAt(_position, _position + _front, _up);
}

glm::mat4
Camera::getProjectionMatrix() const
{
    return glm::perspective(
        glm::radians(_fov),
        float(_viewport.width) / float(_viewport.height),
        Z_NEAR, Z_FAR);
}

void
Camera::updateVectors()
{
     _front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _front.y = sin(glm::radians(_pitch));
    _front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _front = glm::normalize(_front);
    _right = glm::normalize(glm::cross(_front, WORLD_UP));
    _up    = glm::normalize(glm::cross(_right, _front));
}
