#pragma once

#include <glm/glm.hpp>


const glm::vec3 WORLD_UP           = {0.0f, 1.0f, 0.0};
const float     CAMERA_SPEED       = 3.0f;
const float     CAMERA_SENSITIVITY = 0.07f;
const float     CAMERA_FOV         = 70.0f;


// Simple FPS-style camera
class Camera {
public:
    enum MoveDirection {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    struct Viewport {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    Camera();
    Camera(glm::vec3 position, float yaw, float pitch);

    void move(MoveDirection dir, float dt);
    void look(float xoffset, float yoffset);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;

    // Configurable parameters
    float _move_speed  = CAMERA_SPEED;
    float _sensitivity = CAMERA_SENSITIVITY;
    float _fov         = CAMERA_FOV;

    Viewport _viewport;
private:
    void updateVectors();

    glm::vec3 _position;
    float _yaw, _pitch;

    glm::vec3 _front;
    glm::vec3 _up;
    glm::vec3 _right;
};
