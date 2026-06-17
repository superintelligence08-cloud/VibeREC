#include "gui/widgets.h"
#include "clip.h"
#include "timeline.h"

// Dear ImGui
#include "imgui.h"

// Standard library
#include <filesystem>
namespace fs = std::filesystem;

namespace IronCut {

// ============================================================================
// TimelineWidget Implementation
// ============================================================================

TimelineWidget::TimelineWidget()
    : m_zoom(1.0f)
    , m_scrollOffset(0.0f)
    , m_trackHeight(60.0f)
    , m_isDragging(false)
    , m_isResizing(false)
    , m_draggedClipTrack(-1)
    , m_draggedClipIndex(-1)
    , m_dragStartTime(0.0)
{
}

TimelineWidget::~TimelineWidget() {
}

void TimelineWidget::render(Timeline& timeline, int selectedTrack, int selectedClip) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float availWidth = ImGui::GetContentRegionAvail().x;
    float availHeight = ImGui::GetContentRegionAvail().y;
    
    // Time ruler
    float rulerHeight = 30.0f;
    drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + availWidth, cursorPos.y + rulerHeight), 
                            IM_COL32(40, 40, 40, 255));
    
    // Draw time markers
    float pixelsPerSecond = 100.0f * m_zoom;
    int totalSeconds = static_cast<int>(timeline.getTotalDuration().value) + 1;
    
    for (int i = 0; i <= totalSeconds; ++i) {
        float x = cursorPos.x + m_scrollOffset + i * pixelsPerSecond;
        if (x >= cursorPos.x && x <= cursorPos.x + availWidth) {
            // Major marker
            drawList->AddLine(ImVec2(x, cursorPos.y), ImVec2(x, cursorPos.y + rulerHeight * 0.7f), 
                              IM_COL32(200, 200, 200, 255), 1.0f);
            
            // Time label
            char buffer[32];
            int mins = i / 60;
            int secs = i % 60;
            snprintf(buffer, sizeof(buffer), "%d:%02d", mins, secs);
            drawList->AddText(ImVec2(x + 5, cursorPos.y + 5), IM_COL32(200, 200, 200, 255), buffer);
        }
    }
    
    // Playhead - using a local playhead position for now
    static double currentTime = 0.0;
    float playheadX = cursorPos.x + m_scrollOffset + currentTime * pixelsPerSecond;
    drawList->AddLine(ImVec2(playheadX, cursorPos.y + rulerHeight), 
                      ImVec2(playheadX, cursorPos.y + availHeight), 
                      IM_COL32(255, 50, 50, 255), 2.0f);
    
    // Playhead handle
    ImVec2 playheadTop(playheadX - 6, cursorPos.y + rulerHeight);
    ImVec2 playheadBottom(playheadX + 6, cursorPos.y + rulerHeight + 10);
    drawList->AddTriangleFilled(playheadTop, 
                                 ImVec2(playheadX - 6, cursorPos.y + rulerHeight + 10),
                                 ImVec2(playheadX + 6, cursorPos.y + rulerHeight + 10),
                                 IM_COL32(255, 50, 50, 255));
    
    // Tracks area
    float tracksY = cursorPos.y + rulerHeight;
    float tracksHeight = availHeight - rulerHeight;
    
    // Background
    drawList->AddRectFilled(ImVec2(cursorPos.x, tracksY), 
                            ImVec2(cursorPos.x + availWidth, tracksY + tracksHeight), 
                            IM_COL32(30, 30, 30, 255));
    
    // Render each video track
    // For now, we'll just render a placeholder since the Timeline API differs
    // In a real implementation, you'd iterate through actual tracks
    int numTracks = 1; // Placeholder
    for (int i = 0; i < numTracks; ++i) {
        float trackY = tracksY + i * m_trackHeight;
        if (trackY + m_trackHeight > tracksY + tracksHeight) break;
        
        // Create a placeholder track for rendering
        VideoTrack placeholderTrack;
        placeholderTrack.id = i;
        placeholderTrack.muted = false;
        placeholderTrack.locked = false;
        renderTrack(placeholderTrack, i, trackY, m_trackHeight);
        
        // Selection highlight
        if (i == selectedTrack) {
            drawList->AddRect(ImVec2(cursorPos.x, trackY), 
                              ImVec2(cursorPos.x + availWidth, trackY + m_trackHeight), 
                              IM_COL32(100, 150, 255, 255), 0.0f, 0, 2.0f);
        }
    }
    
    // Add track button
    float addButtonY = tracksY + numTracks * m_trackHeight;
    if (addButtonY < tracksY + tracksHeight) {
        if (ImGui::InvisibleButton("AddTrack", ImVec2(availWidth, 30))) {
            // Would call timeline.addVideoTrack() in full implementation
        }
        drawList->AddText(ImVec2(cursorPos.x + 10, addButtonY + 8), 
                          IM_COL32(150, 150, 150, 255), "+ Click to add video track");
    }
    
    // Handle input
    handleInput(timeline, cursorPos.x, tracksY);
    
    // Scrollbar
    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x, tracksY));
    ImGui::ScrollbarHorizontal(ImGui::GetID("TimelineScroll"));
    m_scrollOffset = -ImGui::GetScrollX();
}

void TimelineWidget::renderTrack(const VideoTrack& track, int trackIndex, float y, float height) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();
    float availWidth = ImGui::GetContentRegionAvail().x;
    float pixelsPerSecond = 100.0f * m_zoom;
    
    // Track background
    ImU32 trackColor = IM_COL32(50, 60, 80, 200);
    drawList->AddRectFilled(ImVec2(cursorPos.x, y), 
                            ImVec2(cursorPos.x + availWidth, y + height), 
                            trackColor);
    
    // Track label
    char label[64];
    snprintf(label, sizeof(label), "Video Track %d", trackIndex + 1);
    drawList->AddText(ImVec2(cursorPos.x + 5, y + 5), IM_COL32(180, 180, 180, 255), label);
    
    // Render clips
    for (size_t i = 0; i < track.clips.size(); ++i) {
        const Clip& clip = *track.clips[i];
        float clipX = cursorPos.x + m_scrollOffset + clip.startTime * pixelsPerSecond;
        float clipWidth = clip.duration * pixelsPerSecond;
        
        if (clipX + clipWidth > cursorPos.x && clipX < cursorPos.x + availWidth) {
            bool isSelected = false; // Would need to pass selected clip info
            renderClip(clip, clipX, y + 20, clipWidth, height - 20, isSelected);
        }
    }
}

void TimelineWidget::renderClip(const Clip& clip, float x, float y, float width, float height, bool isSelected) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Clip background
    ImU32 clipColor = isSelected ? IM_COL32(100, 150, 200, 220) : IM_COL32(80, 120, 160, 200);
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), clipColor, 4.0f);
    
    // Clip border
    ImU32 borderColor = isSelected ? IM_COL32(150, 200, 255, 255) : IM_COL32(120, 160, 200, 255);
    drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), borderColor, 4.0f, 0, 2.0f);
    
    // Clip name
    std::string displayName = clip.sourcePath;
    if (displayName.length() > 20) {
        displayName = displayName.substr(0, 17) + "...";
    }
    drawList->AddText(ImVec2(x + 5, y + 5), IM_COL32(255, 255, 255, 255), displayName.c_str());
    
    // Duration
    char durationStr[32];
    snprintf(durationStr, sizeof(durationStr), "%.2fs", clip.duration);
    drawList->AddText(ImVec2(x + 5, y + height - 15), IM_COL32(200, 200, 200, 255), durationStr);
    
    // Resize handles
    float handleWidth = 8.0f;
    drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + handleWidth, y + height), 
                            IM_COL32(180, 180, 180, 200), 2.0f);
    drawList->AddRectFilled(ImVec2(x + width - handleWidth, y), ImVec2(x + width, y + height), 
                            IM_COL32(180, 180, 180, 200), 2.0f);
}

void TimelineWidget::handleInput(Timeline& timeline, float startX, float startY) {
    ImGuiIO& io = ImGui::GetIO();
    
    // Simple click detection for playhead
    if (ImGui::IsMouseClicked(0)) {
        ImVec2 mousePos = io.MousePos;
        float pixelsPerSecond = 100.0f * m_zoom;
        double newTime = (mousePos.x - startX - m_scrollOffset) / pixelsPerSecond;
        // Note: In full implementation, would update the static currentTime in render()
        if (newTime >= 0) {
            // Would call timeline.seekToTime(newTime) in full implementation
        }
    }
}

// ============================================================================
// PreviewWindow Implementation
// ============================================================================

PreviewWindow::PreviewWindow()
    : m_window(nullptr)
    , m_width(1280)
    , m_height(720)
    , m_texture(0)
    , m_initialized(false)
{
}

PreviewWindow::~PreviewWindow() {
    shutdown();
}

bool PreviewWindow::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    
    // Create OpenGL texture
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    
    m_initialized = true;
    return true;
}

void PreviewWindow::shutdown() {
    if (m_texture != 0) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }
    m_initialized = false;
}

void PreviewWindow::render(const uint8_t* frameData, int width, int height) {
    if (!m_initialized) return;
    
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    
    // Update texture if new frame data provided
    if (frameData) {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, frameData);
    }
    
    // Display texture in ImGui
    ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<uintptr_t>(m_texture)), 
                 ImVec2(availSize.x, availSize.y));
}

void PreviewWindow::setSize(int width, int height) {
    m_width = width;
    m_height = height;
    
    if (m_initialized) {
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    }
}

bool PreviewWindow::shouldClose() const {
    return false; // Controlled by main application
}

// ============================================================================
// PropertiesPanel Implementation
// ============================================================================

PropertiesPanel::PropertiesPanel() {
    m_nameBuffer[0] = '\0';
    m_startTime = 0.0f;
    m_duration = 0.0f;
    m_volume = 1.0f;
    m_muted = false;
}

void PropertiesPanel::render(Clip* selectedClip) {
    if (!selectedClip) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No clip selected");
        return;
    }
    
    // Copy clip data to local buffers
    strncpy(m_nameBuffer, selectedClip->getFilePath().c_str(), sizeof(m_nameBuffer) - 1);
    m_nameBuffer[sizeof(m_nameBuffer) - 1] = '\0';
    m_startTime = static_cast<float>(selectedClip->getStartTime().seconds());
    m_duration = static_cast<float>(selectedClip->getDuration().seconds());
    m_volume = selectedClip->getVolume();
    m_muted = selectedClip->isMuted();
    
    ImGui::Text("Clip Properties");
    ImGui::Separator();
    
    if (ImGui::InputText("Name", m_nameBuffer, sizeof(m_nameBuffer))) {
        // Note: Would need setter for filePath in full implementation
    }
    
    ImGui::SliderFloat("Start Time", &m_startTime, 0.0f, 100.0f, "%.2f s");
    // Note: Would call setStartTime with proper RationalTime conversion
    
    ImGui::SliderFloat("Duration", &m_duration, 0.1f, 100.0f, "%.2f s");
    // Note: Would call setDuration with proper RationalTime conversion
    
    ImGui::Separator();
    ImGui::Text("Audio");
    
    ImGui::SliderFloat("Volume", &m_volume, 0.0f, 2.0f, "%.2f");
    selectedClip->setVolume(m_volume);
    
    ImGui::Checkbox("Muted", &m_muted);
    selectedClip->setMuted(m_muted);
    
    ImGui::Separator();
    ImGui::Text("Transform");
    
    static float position[2] = {0.0f, 0.0f};
    static float scale[2] = {1.0f, 1.0f};
    static float rotation = 0.0f;
    
    ImGui::DragFloat2("Position", position, 1.0f);
    ImGui::DragFloat2("Scale", scale, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Rotation", &rotation, 1.0f);
}

void PropertiesPanel::onPropertyChange(std::function<void()> callback) {
    // Callback when properties change
}

// ============================================================================
// MediaBrowser Implementation
// ============================================================================

MediaBrowser::MediaBrowser()
    : m_currentPath(".")
{
    loadDirectory(m_currentPath);
}

void MediaBrowser::render(const std::string& currentPath) {
    if (currentPath != m_currentPath) {
        m_currentPath = currentPath;
        loadDirectory(m_currentPath);
    }
    
    // Navigation buttons
    if (ImGui::Button("..")) {
        fs::path parent = fs::path(m_currentPath).parent_path();
        if (parent.empty()) parent = "/";
        m_currentPath = parent.string();
        loadDirectory(m_currentPath);
    }
    ImGui::SameLine();
    ImGui::Text("%s", m_currentPath.c_str());
    
    ImGui::Separator();
    
    // Directories
    for (const auto& dir : m_directories) {
        if (ImGui::Selectable(dir.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0)) {
                m_currentPath = fs::path(m_currentPath) / dir;
                loadDirectory(m_currentPath);
            }
        }
    }
    
    // Files
    for (const auto& file : m_files) {
        bool isVideo = isVideoFile(file);
        ImVec4 textColor = isVideo ? ImVec4(0.6f, 0.8f, 1.0f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        if (ImGui::Selectable(file.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick)) {
            if (ImGui::IsMouseDoubleClicked(0) && onFileDoubleClicked) {
                std::string fullPath = fs::path(m_currentPath) / file;
                onFileDoubleClicked(fullPath);
            }
        }
        ImGui::PopStyleColor();
    }
}

void MediaBrowser::refresh() {
    loadDirectory(m_currentPath);
}

void MediaBrowser::loadDirectory(const std::string& path) {
    m_directories.clear();
    m_files.clear();
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                m_directories.push_back(entry.path().filename().string());
            } else if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (isVideoFile(filename) || 
                    filename.find(".mp3") != std::string::npos ||
                    filename.find(".wav") != std::string::npos) {
                    m_files.push_back(filename);
                }
            }
        }
    } catch (const std::exception& e) {
        // Directory access error
    }
}

bool MediaBrowser::isVideoFile(const std::string& filename) {
    std::string ext = fs::path(filename).extension().string();
    return ext == ".mp4" || ext == ".mov" || ext == ".avi" || ext == ".mkv" ||
           ext == ".webm" || ext == ".flv" || ext == ".m4v";
}

// ============================================================================
// Toolbar Implementation
// ============================================================================

Toolbar::Toolbar()
    : m_currentTool(Select)
{
}

void Toolbar::render(Tool& selectedTool, bool& isPlaying, float& zoom) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    
    // Select tool
    if (ImGui::RadioButton("Select", m_currentTool == Select)) {
        m_currentTool = Select;
        selectedTool = Select;
    }
    ImGui::SameLine();
    
    // Razor tool
    if (ImGui::RadioButton("Razor", m_currentTool == Razor)) {
        m_currentTool = Razor;
        selectedTool = Razor;
    }
    ImGui::SameLine();
    
    // Hand tool
    if (ImGui::RadioButton("Hand", m_currentTool == Hand)) {
        m_currentTool = Hand;
        selectedTool = Hand;
    }
    ImGui::SameLine();
    
    // Zoom tool
    if (ImGui::RadioButton("Zoom", m_currentTool == Zoom)) {
        m_currentTool = Zoom;
        selectedTool = Zoom;
    }
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Playback controls
    if (ImGui::Button(isPlaying ? "|| Pause" : "> Play", ImVec2(60, 0))) {
        isPlaying = !isPlaying;
    }
    
    ImGui::SameLine();
    if (ImGui::Button("[] Stop", ImVec2(60, 0))) {
        isPlaying = false;
    }
    
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();
    
    // Zoom slider
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##Zoom", &zoom, 0.1f, 5.0f, "Zoom: %.1fx");
    
    ImGui::PopStyleVar();
}

} // namespace IronCut
