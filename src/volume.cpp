#include "volume.hpp"

#include "imgui.h"

#include "camera.hpp"

Volume::Volume(glm::vec3 dims, float resolution, glm::vec3 offset,
               glm::vec2 frame_size) :
    _dims(dims),
    _resolution(resolution),
    _offset(offset),
    _frame_size(frame_size),
    _integrate_shader("res/shaders/tsdf.glsl"),
    _raycast_shader("res/shaders/raycast.vert",
                    "res/shaders/raycast.frag")
{
    _model = glm::mat4(1.0f);

    // Translate then scale (instruction order is reversed due to glm being
    // column major)
    _texture_to_model = glm::scale(
        glm::mat4(1.0f),
        glm::vec3(_dims) * _resolution);
    _texture_to_model = glm::translate(
        _texture_to_model,
        glm::vec3(-0.5f, -0.5f, -0.5f));

    _integrate_shader.use();
    _integrate_shader.setMat4("texture_to_model", _texture_to_model);
    _integrate_shader.setFloat("trunc_margin", _resolution * _trunc_margin);

    _raycast_shader.use();
    _raycast_shader.setInt("tsdf_tex", 0);
    _raycast_shader.setInt("color_tex", 1);

    //--------------------------------------------------------------------------
    // TEXTURES

    glGenTextures(1, &_tsdf_tex);
    glBindTexture(GL_TEXTURE_3D, _tsdf_tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_R16F,
        _dims.x, _dims.y, _dims.z,
        0,
        GL_RED, GL_HALF_FLOAT,
        (void*)0);
    glBindImageTexture(0, _tsdf_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);

    glGenTextures(1, &_color_tex);
    glBindTexture(GL_TEXTURE_3D, _color_tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_RGBA8,
        _dims.x, _dims.y, _dims.z,
        0,
        GL_RGBA, GL_UNSIGNED_BYTE,
        (void*)0);
    glBindImageTexture(1, _color_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

    glGenTextures(1, &_weight_tex);
    glBindTexture(GL_TEXTURE_3D, _weight_tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_R16UI,
        _dims.x, _dims.y, _dims.z,
        0,
        GL_RED_INTEGER, GL_SHORT,
        (void*)0);
    glBindImageTexture(2, _weight_tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16UI);

    glGenTextures(1, &_frame_depth_tex);
    glBindTexture(GL_TEXTURE_2D, _frame_depth_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R16UI,
        _frame_size.x, _frame_size.y,
        0,
        GL_RED_INTEGER, GL_UNSIGNED_SHORT,
        (void*)0);

    glGenTextures(1, &_frame_color_tex);
    glBindTexture(GL_TEXTURE_2D, _frame_color_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB8,
        _frame_size.x, _frame_size.y,
        0,
        GL_RGB, GL_UNSIGNED_BYTE,
        (void*)0);

    reset();
    createVolume();
}

Volume::~Volume()
{
    // XXX
    //glDeleteVertexArrays(1, &volume_vao);
    //glDeleteBuffers(1, &volume_vbo);
}

void
Volume::integrate(const unsigned short *depth_data,
                  const unsigned char  *color_data,
                  const glm::mat4 &intrinsic,
                  const glm::mat4 &extrinsic)
{
    _integrate_shader.use();
    _integrate_shader.setMat4("model", _model);
    _integrate_shader.setMat4("extrinsic", extrinsic);
    _integrate_shader.setMat3("intrinsic", intrinsic);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, _frame_depth_tex);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    _frame_size.x, _frame_size.y,
                    GL_RED_INTEGER, GL_UNSIGNED_SHORT,
                    depth_data);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _frame_color_tex);
    glTexSubImage2D(GL_TEXTURE_2D,
                    0,
                    0, 0,
                    _frame_size.x, _frame_size.y,
                    GL_RGB, GL_UNSIGNED_BYTE,
                    color_data);

    glDispatchCompute(_dims.x / 32, _dims.y / 32, _dims.z);

    // Don't allow other shaders to access the buffers touched by the compute
    // shader until it's done executing
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void
Volume::draw(const Camera *camera)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    _raycast_shader.use();

    glm::mat4 view = camera->getViewMatrix();
    glm::mat4 projection = camera->getProjectionMatrix();
    glm::mat4 mvp = projection * view * _model;
    glm::vec4 camera_pos_tex_space =
        glm::inverse(_texture_to_model) *
        glm::inverse(_model) *
        glm::inverse(view) *
        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    _raycast_shader.setMat4("mvp", mvp);
    _raycast_shader.setVec3("volume_dims", _dims);
    _raycast_shader.setVec3("camera_pos_tex_space",
                          glm::vec3(camera_pos_tex_space));
    _raycast_shader.setFloat("step_size", _step_size);
    _raycast_shader.setInt("display_mode", _display_mode);

    glBindVertexArray(_box_vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, _tsdf_tex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, _color_tex);
    glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, 0);
}

void
Volume::reset()
{
    glClearTexImage(_tsdf_tex, 0, GL_RED, GL_HALF_FLOAT, (void *)0);
    unsigned char clear_color[] = {255, 255, 255, 255};
    glClearTexImage(_color_tex, 0, GL_RGBA, GL_UNSIGNED_BYTE, &clear_color);
    glClearTexImage(_weight_tex, 0, GL_RED_INTEGER, GL_SHORT, (void *)0);
}

// Create a box that will contain the volume.
// The box is centered on its center of gravity.
void
Volume::createVolume()
{
    glm::vec3 min, max;
    min = -(glm::vec3(_dims) * (_resolution * 0.5f));
    max =  (glm::vec3(_dims) * (_resolution * 0.5f));

    static GLfloat vertices[] = {
        // vertices            texcoords
        max.x, max.y, max.z,   1.0f, 1.0f, 1.0f,
        min.x, max.y, max.z,   0.0f, 1.0f, 1.0f,
        max.x, max.y, min.z,   1.0f, 1.0f, 0.0f,
        min.x, max.y, min.z,   0.0f, 1.0f, 0.0f,
        max.x, min.y, max.z,   1.0f, 0.0f, 1.0f,
        min.x, min.y, max.z,   0.0f, 0.0f, 1.0f,
        min.x, min.y, min.z,   0.0f, 0.0f, 0.0f,
        max.x, min.y, min.z,   1.0f, 0.0f, 0.0f
    };
    static GLuint indices[] = {
        3, 2, 6, 7, 4, 2, 0,
        3, 1, 6, 5, 4, 1, 0
    };

    unsigned int vbo, ebo;
    glGenVertexArrays(1, &_box_vao);
    glBindVertexArray(_box_vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(vertices),
                 vertices,
                 GL_STATIC_DRAW);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(indices),
                 indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*)(3 * sizeof(float)));
}
