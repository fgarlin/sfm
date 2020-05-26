#pragma once

#include <string>
#include <vector>

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "camera.hpp"

class Volume;

class App {
public:
    App(int argc, char **argv);
    void run();
private:
    GLFWwindow *_window     = nullptr;

    std::string _dataset_dir;

    float       _delta_time = 0.0f;
    float       _last_time  = 0.0f;

    Camera      _camera;

    Volume     *_volume;

    bool        _paused = true;
    int         _total_frames = 1000;
    int         _current_frame = 0;

    float       _background_color[3] = {1.0f, 1.0f, 1.0f};

    float       _fx = 0.0f;
    float       _fy = 0.0f;
    float       _cx = 0.0f;
    float       _cy = 0.0f;
    float       _s  = 0.0f;

    std::vector<unsigned short *> _depth_frames;
    std::vector<unsigned char *> _color_frames;

    void processCmdArgs(int argc, char **argv);

    void mainLoop();
    void cleanup();

    void initGLFW();
    void initGUI();

    void processInput();
    void drawGUI();
    bool loadDataFrame(int n,
                       unsigned short **depth,
                       unsigned char **color,
                       glm::mat4 *extrinsic);
};
