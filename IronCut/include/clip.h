#ifndef CLIP_H
#define CLIP_H

#include <string>
#include <memory>
#include <cstdint>

namespace IronCut {

enum class MediaType {
    VIDEO,
    AUDIO,
    IMAGE
};

struct RationalTime {
    int64_t value;
    int64_t timescale; // frames per second or sample rate
    
    double seconds() const { return static_cast<double>(value) / timescale; }
};

class Clip {
public:
    Clip(const std::string& filePath, MediaType type);
    ~Clip();

    // Getters
    std::string getFilePath() const { return filePath_; }
    MediaType getType() const { return type_; }
    RationalTime getDuration() const { return duration_; }
    RationalTime getStartTime() const { return startTime_; }
    RationalTime getEndTime() const { return endTime_; }
    
    // Setters for timeline positioning
    void setStartTime(RationalTime time) { 
        startTime_ = time; 
        calculateEndTime();
    }
    
    void setDuration(RationalTime duration) {
        duration_ = duration;
        calculateEndTime();
    }
    
    // Audio properties
    float getVolume() const { return volume_; }
    void setVolume(float vol) { volume_ = vol; }
    bool isMuted() const { return muted_; }
    void setMuted(bool m) { muted_ = m; }

    // Trim operations
    void trimIn(int64_t frames);
    void trimOut(int64_t frames);

private:
    std::string filePath_;
    MediaType type_;
    RationalTime duration_;
    RationalTime startTime_;
    RationalTime endTime_;
    float volume_;  // 0.0 to 2.0
    bool muted_;    // mute flag
    
    void calculateEndTime();
};

using ClipPtr = std::shared_ptr<Clip>;

} // namespace IronCut

#endif // CLIP_H
