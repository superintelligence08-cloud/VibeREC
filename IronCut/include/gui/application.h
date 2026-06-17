#pragma once

#include "widgets.h"
#include "timeline.h"
#include "engine.h"
#include <string>
#include <memory>

namespace IronCut {

class Application {
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void shutdown();

    // Menu callbacks
    void newProject();
    void openProject(const std::string& path);
    void saveProject(const std::string& path);
    void importMedia(const std::string& path);
    void exportProject(const std::string& path, const ExportSettings& settings);

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    
    // Core components
    TimelinePtr m_timeline;
    EnginePtr m_engine;
    
    // UI Components
    TimelineWidget m_timelineWidget;
    PreviewWindow m_previewWindow;
    PropertiesPanel m_propertiesPanel;
    MediaBrowser m_mediaBrowser;
    Toolbar m_toolbar;
    
    // State
    bool m_isRunning;
    bool m_isPlaying;
    int m_selectedTrack;
    int m_selectedClip;
    float m_zoom;
    std::string m_currentProjectPath;
    
    // Render loop
    void renderFrame();
    void processEvents();
    void updatePlayback();
    
    // UI Layout
    void renderMenuBar();
    void renderMainWorkspace();
    
    // Helpers
    void showFileDialog(bool isOpen, const std::string& title);
    void showAboutDialog();
    void showExportDialog();
};

} // namespace IronCut
