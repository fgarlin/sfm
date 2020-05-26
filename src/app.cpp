#include "app.hpp"

#include <fstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "imgui.h"
#include "imgui_internal.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"

#include "stb_image.h"

#include "shader.hpp"
#include "volume.hpp"


const glm::uvec2 SCREEN_SIZE        = {1280, 720};

// Dimensions of the volume in voxels
const glm::uvec3 VOLUME_DIMS        = {512, 512, 512};
// Size of each voxel in meters
const float      VOLUME_RESOLUTION  = 0.02f;

const glm::uvec2 DATASET_FRAME_SIZE = {640, 480};


App::App(int argc, char **argv) :
    _fx(585.0f),
    _fy(585.0f),
    _cx(DATASET_FRAME_SIZE.x / 2.0f),
    _cy(DATASET_FRAME_SIZE.y / 2.0f)
{
    processCmdArgs(argc, argv);
}

void
App::run()
{
    initGLFW();
    initGUI();
    mainLoop();
    cleanup();
}

void
App::processCmdArgs(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "No dataset directory specified" << std::endl;
        exit(0);
    }

    _dataset_dir = argv[1];
}

void
App::mainLoop()
{
    _volume = new Volume(VOLUME_DIMS,
                         VOLUME_RESOLUTION,
                         glm::vec3(0.0f, 0.0f, 0.0f),
                         DATASET_FRAME_SIZE);

    while (!glfwWindowShouldClose(_window)) {
        float current_time = glfwGetTime();
        _delta_time = current_time - _last_time;
        _last_time = current_time;

        processInput();

        drawGUI();

        glfwGetWindowSize(_window,
                          &_camera._viewport.width,
                          &_camera._viewport.height);
        glViewport(0, 0, _camera._viewport.width, _camera._viewport.height);
        glClearColor(_background_color[0],
                     _background_color[1],
                     _background_color[2],
                     1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (!_paused) {
            if (_current_frame < _total_frames) {
                unsigned short *depth_data = nullptr;
                unsigned char  *color_data = nullptr;
                glm::mat4 extrinsic(1.0f);
                glm::mat3 intrinsic(0.0f);
                intrinsic[0][0] = _fx;
                intrinsic[1][1] = _fy;
                intrinsic[2][0] = _cx;
                intrinsic[2][1] = _cy;
                intrinsic[1][0] = _s;
                intrinsic[2][2] = 1.0f;

                bool data_received = loadDataFrame(_current_frame,
                                                   &depth_data,
                                                   &color_data,
                                                   &extrinsic);
                if (data_received) {
                    _volume->integrate(depth_data, color_data, intrinsic, extrinsic);
                    stbi_image_free(depth_data);
                    stbi_image_free(color_data);
                    ++_current_frame;
                }
            }
        }

        _volume->draw(&_camera);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(_window);
        glfwPollEvents();
    }

    delete _volume;
}

void
App::cleanup()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(_window);
    glfwTerminate();
}

void
App::initGLFW()
{
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    _window = glfwCreateWindow(SCREEN_SIZE.x, SCREEN_SIZE.y,
                                "Real-time Structure from Motion",
                                NULL, NULL);
    if (!_window)
        throw std::runtime_error("Failed to create GLFW window");
    glfwMakeContextCurrent(_window);
    if(!gladLoadGL())
        throw std::runtime_error("Failed to initialize OpenGL loader (glad)");
    glfwSwapInterval(1);

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
	std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
              << std::endl;
	std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
}

void
App::initGUI()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 450");
    ImGui::StyleColorsLight();
}

void
App::processInput()
{
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(_window, GLFW_TRUE);
    if (glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS)
        _camera.move(Camera::FORWARD, _delta_time);
    if (glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS)
        _camera.move(Camera::LEFT, _delta_time);
    if (glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS)
        _camera.move(Camera::BACKWARD, _delta_time);
    if (glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS)
        _camera.move(Camera::RIGHT, _delta_time);
    if (glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS)
        _camera.move(Camera::UP, _delta_time);
    if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        _camera.move(Camera::DOWN, _delta_time);

    if (ImGui::GetIO().WantCaptureMouse)
        return;

    double xpos, ypos;
    glfwGetCursorPos(_window, &xpos, &ypos);

    static bool initialized = false;
    static double lastX, lastY;
    if (initialized) {
        lastX = xpos;
        lastY = ypos;
        initialized = true;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    int leftclick = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);
    if (leftclick == GLFW_PRESS) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        _camera.look(xoffset, yoffset);
    } else {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}


void
App::drawGUI()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_area_pos = viewport->GetWorkPos();
    ImVec2 work_area_size = viewport->GetWorkSize();

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f),
                            ImGuiCond_FirstUseEver,
                            ImVec2(0.0f, 0.0f));
    ImGui::Begin("Options");
    if (ImGui::CollapsingHeader("Navigation Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::SliderFloat("Movespeed", &_camera._move_speed,
                           0.1f, 10.0f, "%.1f");
        ImGui::SliderFloat("Sensitivity", &_camera._sensitivity,
                           0.01f, 0.5f, "%.2f");
        ImGui::SliderFloat("FOV", &_camera._fov,
                           30.0f, 120.0f, "%.1f");
        ImGui::PopItemWidth();
    }
    if (ImGui::CollapsingHeader("Display Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::ColorEdit3("Background Color", _background_color,
                          ImGuiColorEditFlags_NoInputs);
        float step_size = _volume->getStepSize();
        ImGui::SliderFloat("Step Size", &step_size, 0.001f,
                           VOLUME_RESOLUTION / 2.0f, "%.3f");
        _volume->setStepSize(step_size);
        ImGui::PopItemWidth();

        ImGui::Separator();
        ImGui::Text("Display Mode");
        int display_mode = _volume->getDisplayMode();
        ImGui::RadioButton("True Color", &display_mode, 0);
        ImGui::RadioButton("Normals", &display_mode, 1);
        ImGui::RadioButton("Phong Shading", &display_mode, 2);
        _volume->setDisplayMode(display_mode);
    }
    if (ImGui::CollapsingHeader("Intrinsic Parameters",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Pinhole camera model");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
        ImGui::SliderFloat("fx", &_fx, 0.0f, 1000.0f, "%.3f");
        static bool tie_focal_length = true;
        ImGui::SameLine();
        ImGui::Checkbox("tie", &tie_focal_length);
        if (tie_focal_length)
            _fy = _fx;
        ImGui::SliderFloat("fy", &_fy, 0.0f, 1000.0f, "%.3f");
        if (tie_focal_length)
            _fx = _fy;
        ImGui::SliderFloat("cx", &_cx, 0.0f, float(DATASET_FRAME_SIZE.x), "%.3f");
        ImGui::SliderFloat("cy", &_cy, 0.0f, float(DATASET_FRAME_SIZE.y), "%.3f");
        ImGui::SliderFloat("skew factor", &_s, -float(DATASET_FRAME_SIZE.x), float(DATASET_FRAME_SIZE.x), "%.3f");
        ImGui::PopItemWidth();
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(200,-1));
    ImGui::SetNextWindowPos(
        ImVec2(work_area_pos.x + work_area_size.x - 10,
               work_area_pos.y + work_area_size.y - 10),
        ImGuiCond_FirstUseEver,
        ImVec2(1.0f, 1.0f));
    ImGui::Begin("Integration Controls", nullptr, ImGuiWindowFlags_NoResize);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
    if (ImGui::Button("Play", ImVec2(ImGui::GetContentRegionAvail().x * 0.7f, 25.0f)))
        _paused = false;
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.545f, 0.0f, 0.0f, 1.0f));
    if (ImGui::Button("Stop", ImVec2(ImGui::GetContentRegionAvail().x, 25.0f)))
        _paused = true;
    ImGui::PopStyleColor();
    if (_paused)
        ImGui::Text("Stopped");
    else
        ImGui::Text("Playing...");
    ImGui::Text("Frame %i of %i", std::min(_current_frame + 1, _total_frames),
                _total_frames);
    ImGui::Text("%.0f fps, %.2f ms",
                ImGui::GetIO().Framerate,
                1000.0f / ImGui::GetIO().Framerate);
    ImGui::Separator();
    if (ImGui::Button("Return to first frame", ImVec2(-1, 0)))
        _current_frame = 0;
    if (ImGui::Button("Reset volume", ImVec2(-1, 0)))
        _volume->reset();
    ImGui::End();
}

bool
App::loadDataFrame(int n,
                   unsigned short **depth,
                   unsigned char **color,
                   glm::mat4 *extrinsic)
{
    std::string base_filename(_dataset_dir + "/frame-");
    char frame_number[7];
    std::snprintf(frame_number, sizeof(frame_number), "%06d", n);
    base_filename += frame_number;

    int width = 0, height = 0, channels = 0;

    std::string depth_filename(base_filename + ".depth.png");
    *depth = stbi_load_16(
        depth_filename.c_str(), &width, &height, &channels, 0);
    if (!(*depth)) {
        std::cerr << "Failed to read depth image from file '"
                  << depth_filename << "'. Skipping frame..." << std::endl;
        return false;
    }
    if (width != DATASET_FRAME_SIZE.x || height != DATASET_FRAME_SIZE.y) {
        std::cerr << "Depth image '" << depth_filename << "' has size "
                  << width << "x" << height
                  << " but expected "
                  << DATASET_FRAME_SIZE.x << "x" << DATASET_FRAME_SIZE.y
                  << ". Skipping frame... " << std::endl;
        return false;
    }
    if (channels != 1) {
        std::cerr << "Depth image '" << depth_filename << "' has "
                  << channels << " channel(s) but expected 1. Skipping frame..."
                  << std::endl;
        return false;
    }

    std::string color_filename(base_filename + ".color.png");
    *color = stbi_load(
        color_filename.c_str(), &width, &height, &channels, 0);
    if (!(*color)) {
        std::cerr << "Failed to read color image from file '"
                  << color_filename << "'. Skipping frame..." << std::endl;
        return false;
    }
    if (width != DATASET_FRAME_SIZE.x || height != DATASET_FRAME_SIZE.y) {
        std::cerr << "Color image '" << color_filename << "' has size "
                  << width << "x" << height
                  << " but expected "
                  << DATASET_FRAME_SIZE.x << "x" << DATASET_FRAME_SIZE.y
                  << ". Skipping frame... " << std::endl;
        return false;
    }
    if (channels != 3) {
        std::cerr << "Color image '" << color_filename << "' has "
                  << channels << " channel(s) but expected 3. Skipping frame..."
                  << std::endl;
        ++_current_frame;
        return false;
    }

    std::string pose_filename(base_filename + ".pose.txt");
    std::ifstream pose_ifs(pose_filename);
    if (!pose_ifs) {
        std::cerr << "Failed to read pose matrix from file '"
                  << pose_filename << "'. Skipping frame..." << std::endl;
        return false;
    }
    float pose_floats[16];
    for (int i = 0; i < 16; ++i)
        pose_ifs >> pose_floats[i];
    // Transpose because glm uses column major ordering
    glm::mat4 pose_matrix = glm::transpose(glm::make_mat4(pose_floats));
    // The extrinsic matrix is the inverse of the camera pose
    *extrinsic = glm::inverse(pose_matrix);

    return true;
}
