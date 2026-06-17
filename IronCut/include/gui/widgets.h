#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "../timeline.h"

struct GLFWwindow;

namespace IronCut {

// Forward declarations for Dear ImGui
typedef void (*ImDrawCallback)(const struct ImDrawList* cmd_list, const struct ImDrawCmd* cmd);

class TimelineWidget {
public:
    TimelineWidget();
    ~TimelineWidget();

    void render(Timeline& timeline, int selectedTrack, int selectedClip);
    void setZoom(float zoom) { m_zoom = zoom; }
    float getZoom() const { return m_zoom; }
    
    // Callbacks
    std::function<void(int track, int clip)> onClipSelected;
    std::function<void(int track, double time)> onPlayheadMoved;
    std::function<void()> onClipDoubleClicked;

private:
    float m_zoom;
    float m_scrollOffset;
    float m_trackHeight;
    bool m_isDragging;
    bool m_isResizing;
    int m_draggedClipTrack;
    int m_draggedClipIndex;
    double m_dragStartTime;
    
    void renderTrack(const VideoTrack& track, int trackIndex, float y, float height);
    void renderClip(const Clip& clip, float x, float y, float width, float height, bool isSelected);
    void handleInput(Timeline& timeline, float startX, float startY);
};

class PreviewWindow {
public:
    PreviewWindow();
    ~PreviewWindow();

    bool initialize(int width, int height);
    void shutdown();
    void render(const uint8_t* frameData, int width, int height);
    void setSize(int width, int height);
    
    bool shouldClose() const;
    
private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    unsigned int m_texture;
    bool m_initialized;
};

class PropertiesPanel {
public:
    void render(Clip* selectedClip);
    void onPropertyChange(std::function<void()> callback);
    
private:
    char m_nameBuffer[256];
    float m_startTime;
    float m_duration;
    float m_volume;
    bool m_muted;
};

class MediaBrowser {
public:
    MediaBrowser();
    
    void render(const std::string& currentPath);
    void refresh();
    
    std::function<void(const std::string& path)> onFileDoubleClicked;
    
private:
    std::string m_currentPath;
    std::vector<std::string> m_files;
    std::vector<std::string> m_directories;
    
    void loadDirectory(const std::string& path);
    bool isVideoFile(const std::string& filename);
};

class Toolbar {
public:
    enum Tool {
        Select,
        Razor,
        Hand,
        Zoom
    };
    
    Toolbar();
    void render(Tool& selectedTool, bool& isPlaying, float& zoom);
    
private:
    Tool m_currentTool;
};

} // namespace IronCut
