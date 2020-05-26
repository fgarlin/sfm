#pragma once

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"

class Camera;

class Volume {
public:
    Volume(glm::vec3 dims, float resolution, glm::vec3 offset,
           glm::vec2 frame_size);
    ~Volume();

    void integrate(const unsigned short *depth_data,
                   const unsigned char  *color_data,
                   const glm::mat4 &intrinsic,
                   const glm::mat4 &extrinsic);
    void draw(const Camera *camera);
    void reset();

    void setStepSize(float step_size) { _step_size = step_size; }
    float getStepSize() const { return _step_size; }

    void setTruncMargin(float trunc_margin) { _trunc_margin = trunc_margin; }
    float getTruncMargin() const { return _trunc_margin; }

    void setDisplayMode(int display_mode) { _display_mode = display_mode; }
    int getDisplayMode() const { return _display_mode; }

    GLuint getFrameColorTexture() const { return _frame_color_tex; }

private:
    void createVolume();

    glm::vec3 _dims;
    float     _resolution;
    glm::vec3 _offset;
    glm::vec2 _frame_size;

    GLuint    _box_vao;

    Shader    _integrate_shader;
    Shader    _raycast_shader;

    glm::mat4 _texture_to_model;
    glm::mat4 _model;

    GLuint    _tsdf_tex;
    GLuint    _color_tex;
    GLuint    _weight_tex;

    GLuint    _frame_depth_tex;
    GLuint    _frame_color_tex;

    float     _step_size = 0.001f;
    float     _trunc_margin = 2.0f;

    // 0 = true color, 1 = normals, 2 = phong shading
    int       _display_mode = 0;
};
