#ifndef ENGINE_H
#define ENGINE_H

#include "timeline.h"
#include <string>
#include <functional>
#include <atomic>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    #include <libswresample/swresample.h>
}

namespace IronCut {

enum class RenderQuality {
    LOW,      // Fast preview
    MEDIUM,   // Standard export
    HIGH,     // Best quality
    PRORES    // Lossless intermediate
};

struct ExportSettings {
    std::string outputPath;
    std::string videoCodec = "libx264";
    std::string audioCodec = "aac";
    int videoBitrate = 8000000; // 8 Mbps
    int audioBitrate = 192000;  // 192 kbps
    int keyframeInterval = 48;  // Every 2 seconds at 24fps
    RenderQuality quality = RenderQuality::HIGH;
    bool twoPass = true;
};

class Engine {
public:
    Engine();
    ~Engine();

    // Initialize engine with timeline
    bool initialize(TimelinePtr timeline);
    
    // Playback control
    bool play();
    bool pause();
    bool stop();
    bool seek(int64_t frame);
    int64_t getCurrentFrame() const { return currentFrame_; }
    bool isPlaying() const { return isPlaying_; }
    
    // Frame acquisition (for preview)
    uint8_t* getFrameRGB(int& width, int& height);
    
    // Export
    bool exportVideo(const ExportSettings& settings);
    void cancelExport();
    bool isExporting() const { return isExporting_; }
    float getExportProgress() const { return exportProgress_; }
    
    // Callbacks
    using ProgressCallback = std::function<void(float progress, const std::string& status)>;
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }
    
    // Error handling
    std::string getLastError() const { return lastError_; }

private:
    TimelinePtr timeline_;
    
    // FFmpeg contexts for decoding
    AVFormatContext* inputFormatCtx_;
    AVCodecContext* videoDecoderCtx_;
    AVCodecContext* audioDecoderCtx_;
    
    // Scaling context
    SwsContext* swsCtx_;
    
    // Playback state
    std::atomic<bool> isPlaying_;
    std::atomic<int64_t> currentFrame_;
    std::atomic<bool> isExporting_;
    std::atomic<float> exportProgress_;
    
    // Current frame buffer
    uint8_t* frameBuffer_;
    int frameWidth_;
    int frameHeight_;
    
    // Callbacks
    ProgressCallback progressCallback_;
    std::string lastError_;
    
    // Internal methods
    bool openMediaFile(const std::string& filePath);
    bool decodeNextFrame();
    bool writeOutputFrame(AVFrame* frame, AVFormatContext* outFmtCtx, AVCodecContext* encCtx);
    void cleanup();
};

using EnginePtr = std::shared_ptr<Engine>;

} // namespace IronCut

#endif // ENGINE_H
