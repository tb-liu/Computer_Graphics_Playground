#pragma once
#include "SystemBase.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLFW_INCLUDE_VULKAN

enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

enum CameraRotate
{
    ROTATE_LEFT,
    UP,
    ROTATE_RIGHT,
    DOWN
};

// default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 15.5f;
const float SENSITIVITY = 1.5f;
const float ZOOM = 45.0f;

class Camera : public SystemBase
{
public:
    void init() override;
    void update(float dt)override;
    void shutdown() override;
    SystemType Type() const override { return SystemType::CAMERA; };

    Camera(glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f),
        glm::vec3 up = glm::vec3(0.f, 1.f, 0.f),
        float yaw_ = YAW, float pitch_ = PITCH);

    glm::mat4 getViewMatrix();

    void processKeys(CameraMovement dir, float dt);

    // use arrow key to 
    void processCameraRotate(CameraRotate dir, float dt);

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // change the move speed
    void processMouseScroll(float yoffset);


private:
    glm::vec3 position, front, up, right, worldUp;
    float yaw, pitch;
    float movementSpeed, mouseSensitivity, zoom;

    void updateCameraVector()
    {
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(front);
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

    void processInput(float deltaTime);
};