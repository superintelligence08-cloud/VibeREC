#include "gui/application.h"
#include "gui/widgets.h"
#include "clip.h"
#include "timeline.h"
#include "engine.h"

// GLFW - must come first before any OpenGL headers
#include <GLFW/glfw3.h>

// Dear ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Standard libraries
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

namespace fs = std::filesystem;

namespace IronCut {

Application::Application()
    : m_window(nullptr)
    , m_width(1920)
    , m_height(1080)
    , m_timeline(nullptr)
    , m_engine(nullptr)
    , m_isRunning(false)
    , m_isPlaying(false)
    , m_selectedTrack(-1)
    , m_selectedClip(-1)
    , m_zoom(1.0f)
{
}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, "IronCut - Professional Video Editor", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    
    io.Fonts->AddFontDefault();
    // Docking enabled via ImGui backend

    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    m_timeline = std::make_shared<Timeline>(1920, 1080, 30.0);
    
    m_engine = std::make_shared<Engine>();
    if (!m_engine->initialize(m_timeline)) {
        std::cerr << "Failed to initialize render engine" << std::endl;
        return false;
    }

    if (!m_previewWindow.initialize(1280, 720)) {
        std::cerr << "Failed to initialize preview window" << std::endl;
        return false;
    }

    m_isRunning = true;
    return true;
}

void Application::run() {
    while (m_isRunning && !glfwWindowShouldClose(m_window)) {
        processEvents();
        
        if (m_isPlaying) {
            updatePlayback();
        }
        
        renderFrame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Application::shutdown() {
    m_previewWindow.shutdown();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    
    glfwTerminate();
    m_isRunning = false;
}

void Application::processEvents() {
    glfwPollEvents();
}

void Application::updatePlayback() {
    static auto lastFrameTime = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration<double>(now - lastFrameTime).count();
    
    if (delta >= 1.0 / 30.0) {
        lastFrameTime = now;
    }
}

void Application::renderFrame() {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    renderMenuBar();
    renderMainWorkspace();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(m_window);
}

void Application::renderMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New Project", "Ctrl+N")) {
                newProject();
            }
            if (ImGui::MenuItem("Open Project...", "Ctrl+O")) {
                showFileDialog(true, "Open Project");
            }
            if (ImGui::MenuItem("Save Project", "Ctrl+S")) {
                saveProject(m_currentProjectPath);
            }
            if (ImGui::MenuItem("Export...", "Ctrl+E")) {
                showExportDialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
                m_isRunning = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Playback")) {
            if (ImGui::MenuItem(m_isPlaying ? "Pause" : "Play", "Space")) {
                m_isPlaying = !m_isPlaying;
            }
            if (ImGui::MenuItem("Stop", nullptr)) {
                m_isPlaying = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About IronCut")) {
                showAboutDialog();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Application::renderMainWorkspace() {
    ImGui::Begin("Preview");
    uint8_t* frameData = nullptr;
    int width = 0, height = 0;
    if (m_engine) {
        frameData = m_engine->getFrameRGB(width, height);
    }
    m_previewWindow.render(frameData, width, height);
    
    if (m_engine) {
        ImGui::Text("Frame: %ld", m_engine->getCurrentFrame());
    }
    if (m_timeline) {
        auto duration = m_timeline->getTotalDuration();
        ImGui::Text("Duration: %.2f s", duration.seconds());
    }
    ImGui::End();

    ImGui::Begin("Media Browser");
    m_mediaBrowser.render(m_currentProjectPath.empty() ? "." : m_currentProjectPath);
    m_mediaBrowser.onFileDoubleClicked = [this](const std::string& path) {
        importMedia(path);
    };
    ImGui::End();

    ImGui::Begin("Timeline");
    
    Toolbar::Tool selectedTool = Toolbar::Select;
    m_toolbar.render(selectedTool, m_isPlaying, m_zoom);
    
    ImGui::Separator();
    
    if (m_timeline) {
        m_timelineWidget.render(*m_timeline, m_selectedTrack, m_selectedClip);
    }
    
    m_timelineWidget.onClipSelected = [this](int track, int clip) {
        m_selectedTrack = track;
        m_selectedClip = clip;
    };
    
    ImGui::End();

    ImGui::Begin("Properties");
    Clip* selectedClipPtr = nullptr;
    if (m_timeline && m_selectedTrack >= 0 && m_selectedClip >= 0) {
        auto clips = m_timeline->getClipsAtFrame(0);
        if (!clips.empty() && m_selectedClip < (int)clips.size()) {
            selectedClipPtr = clips[m_selectedClip].get();
        }
    }
    m_propertiesPanel.render(selectedClipPtr);
    ImGui::End();
}

void Application::newProject() {
    m_timeline = std::make_shared<Timeline>(1920, 1080, 30.0);
    m_currentProjectPath.clear();
    m_selectedTrack = -1;
    m_selectedClip = -1;
    m_zoom = 1.0f;
    if (m_engine) {
        m_engine->initialize(m_timeline);
    }
}

void Application::openProject(const std::string& path) {
    m_currentProjectPath = path;
}

void Application::saveProject(const std::string& path) {
    m_currentProjectPath = path;
}

void Application::importMedia(const std::string& path) {
    if (!fs::exists(path)) {
        std::cerr << "File not found: " << path << std::endl;
        return;
    }

    std::string ext = fs::path(path).extension().string();
    bool isVideo = (ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".mkv");
    bool isAudio = (ext == ".mp3" || ext == ".wav" || ext == ".aac" || ext == ".flac");

    if (isVideo || isAudio) {
        MediaType type = isVideo ? MediaType::VIDEO : MediaType::AUDIO;
        auto clip = std::make_shared<Clip>(path, type);
        
        if (m_timeline) {
            if (isVideo) {
                int trackId = m_timeline->addVideoTrack();
                RationalTime startTime = {0, 30};
                m_timeline->addClipToVideoTrack(trackId, clip, startTime.value);
            } else {
                int trackId = m_timeline->addAudioTrack();
                RationalTime startTime = {0, 30};
                m_timeline->addClipToAudioTrack(trackId, clip, startTime.value);
            }
        }
    } else {
        std::cerr << "Unsupported file format: " << ext << std::endl;
    }
}

void Application::exportProject(const std::string& path, const ExportSettings& settings) {
    ExportSettings exportSettings = settings;
    exportSettings.outputPath = path;
    
    if (m_engine) {
        m_engine->exportVideo(exportSettings);
    }
}

void Application::showFileDialog(bool isOpen, const std::string& title) {
    std::cout << "File dialog: " << title << " (isOpen=" << isOpen << ")" << std::endl;
}

void Application::showAboutDialog() {
    ImGui::OpenPopup("About IronCut");
    
    if (ImGui::BeginPopupModal("About IronCut", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("IronCut v1.0.0");
        ImGui::Separator();
        ImGui::Text("Professional Video Editor");
        ImGui::Text("Built with C++, Dear ImGui, and FFmpeg");
        ImGui::Separator();
        ImGui::Text("© 2024 IronCut Team");
        
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Application::showExportDialog() {
    ImGui::OpenPopup("Export Settings");
    
    if (ImGui::BeginPopupModal("Export Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char filename[256] = "output.mp4";
        static int bitrate = 10;
        
        ImGui::InputText("Filename", filename, IM_ARRAYSIZE(filename));
        ImGui::SliderInt("Bitrate (Mbps)", &bitrate, 1, 50);
        
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            ExportSettings settings;
            settings.videoBitrate = bitrate * 1000000;
            
            exportProject(filename, settings);
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
}

} // namespace IronCut

int main(int argc, char** argv) {
    IronCut::Application app;
    
    if (!app.initialize()) {
        return 1;
    }
    
    app.run();
    app.shutdown();
    
    return 0;
}
