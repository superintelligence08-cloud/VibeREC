#ifndef TIMELINE_H
#define TIMELINE_H

#include "clip.h"
#include <vector>
#include <map>
#include <mutex>

namespace IronCut {

struct VideoTrack {
    int id;
    std::vector<ClipPtr> clips;
    bool muted;
    bool locked;
};

struct AudioTrack {
    int id;
    std::vector<ClipPtr> clips;
    float volume; // 0.0 to 1.0
    bool muted;
    bool locked;
};

class Timeline {
public:
    Timeline(int width, int height, double fps);
    ~Timeline();

    // Timeline properties
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    double getFPS() const { return fps_; }
    RationalTime getTotalDuration() const { return totalDuration_; }

    // Track management
    int addVideoTrack();
    int addAudioTrack();
    void removeVideoTrack(int trackId);
    void removeAudioTrack(int trackId);

    // Clip management
    bool addClipToVideoTrack(int trackId, ClipPtr clip, int64_t startTime);
    bool addClipToAudioTrack(int trackId, ClipPtr clip, int64_t startTime);
    void removeClipFromVideoTrack(int trackId, int clipIndex);
    void removeClipFromAudioTrack(int trackId, int clipIndex);

    // Clip operations
    ClipPtr splitVideoClip(int trackId, int clipIndex, int64_t framePosition);
    ClipPtr splitAudioClip(int trackId, int clipIndex, int64_t framePosition);

    // Query
    std::vector<ClipPtr> getClipsAtFrame(int64_t frame);
    
    // Update total duration based on clips
    void recalculateDuration();

private:
    int width_;
    int height_;
    double fps_;
    RationalTime totalDuration_;
    
    std::vector<VideoTrack> videoTracks_;
    std::vector<AudioTrack> audioTracks_;
    
    int nextVideoTrackId_;
    int nextAudioTrackId_;
    
    mutable std::mutex mutex_;
};

using TimelinePtr = std::shared_ptr<Timeline>;

} // namespace IronCut

#endif // TIMELINE_H
