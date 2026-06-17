#include "timeline.h"
#include <algorithm>
#include <stdexcept>

namespace IronCut {

Timeline::Timeline(int width, int height, double fps)
    : width_(width)
    , height_(height)
    , fps_(fps)
    , totalDuration_{0, static_cast<int64_t>(fps)}
    , nextVideoTrackId_(0)
    , nextAudioTrackId_(0)
{
}

Timeline::~Timeline() {
}

int Timeline::addVideoTrack() {
    std::lock_guard<std::mutex> lock(mutex_);
    VideoTrack track;
    track.id = nextVideoTrackId_++;
    track.muted = false;
    track.locked = false;
    videoTracks_.push_back(track);
    return track.id;
}

int Timeline::addAudioTrack() {
    std::lock_guard<std::mutex> lock(mutex_);
    AudioTrack track;
    track.id = nextAudioTrackId_++;
    track.volume = 1.0f;
    track.muted = false;
    track.locked = false;
    audioTracks_.push_back(track);
    return track.id;
}

void Timeline::removeVideoTrack(int trackId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(videoTracks_.begin(), videoTracks_.end(),
        [trackId](const VideoTrack& t) { return t.id == trackId; });
    
    if (it != videoTracks_.end()) {
        videoTracks_.erase(it);
        recalculateDuration();
    }
}

void Timeline::removeAudioTrack(int trackId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::find_if(audioTracks_.begin(), audioTracks_.end(),
        [trackId](const AudioTrack& t) { return t.id == trackId; });
    
    if (it != audioTracks_.end()) {
        audioTracks_.erase(it);
        recalculateDuration();
    }
}

bool Timeline::addClipToVideoTrack(int trackId, ClipPtr clip, int64_t startTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& track : videoTracks_) {
        if (track.id == trackId) {
            if (track.locked) return false;
            
            clip->setStartTime(RationalTime{startTime, static_cast<int64_t>(fps_)});
            track.clips.push_back(clip);
            
            // Sort clips by start time
            std::sort(track.clips.begin(), track.clips.end(),
                [](const ClipPtr& a, const ClipPtr& b) {
                    return a->getStartTime().value < b->getStartTime().value;
                });
            
            recalculateDuration();
            return true;
        }
    }
    return false;
}

bool Timeline::addClipToAudioTrack(int trackId, ClipPtr clip, int64_t startTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& track : audioTracks_) {
        if (track.id == trackId) {
            if (track.locked) return false;
            
            clip->setStartTime(RationalTime{startTime, static_cast<int64_t>(fps_)});
            track.clips.push_back(clip);
            
            // Sort clips by start time
            std::sort(track.clips.begin(), track.clips.end(),
                [](const ClipPtr& a, const ClipPtr& b) {
                    return a->getStartTime().value < b->getStartTime().value;
                });
            
            recalculateDuration();
            return true;
        }
    }
    return false;
}

void Timeline::removeClipFromVideoTrack(int trackId, int clipIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& track : videoTracks_) {
        if (track.id == trackId) {
            if (track.locked) return;
            
            if (clipIndex >= 0 && clipIndex < static_cast<int>(track.clips.size())) {
                track.clips.erase(track.clips.begin() + clipIndex);
                recalculateDuration();
            }
            return;
        }
    }
}

void Timeline::removeClipFromAudioTrack(int trackId, int clipIndex) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& track : audioTracks_) {
        if (track.id == trackId) {
            if (track.locked) return;
            
            if (clipIndex >= 0 && clipIndex < static_cast<int>(track.clips.size())) {
                track.clips.erase(track.clips.begin() + clipIndex);
                recalculateDuration();
            }
            return;
        }
    }
}

ClipPtr Timeline::splitVideoClip(int trackId, int clipIndex, int64_t framePosition) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& track : videoTracks_) {
        if (track.id == trackId) {
            if (track.locked) return nullptr;
            
            if (clipIndex >= 0 && clipIndex < static_cast<int>(track.clips.size())) {
                ClipPtr original = track.clips[clipIndex];
                
                // Check if split point is within clip
                auto startTime = original->getStartTime();
                auto duration = original->getDuration();
                int64_t endFrame = startTime.value + duration.value;
                
                if (framePosition <= startTime.value || framePosition >= endFrame) {
                    return nullptr; // Split point outside clip
                }
                
                // Create new clip for second part
                ClipPtr secondPart = std::make_shared<Clip>(
                    original->getFilePath(), 
                    original->getType()
                );
                
                // Calculate durations
                int64_t firstDuration = framePosition - startTime.value;
                int64_t secondDuration = endFrame - framePosition;
                
                // Update original clip
                original->setDuration(RationalTime{firstDuration, startTime.timescale});
                
                // Setup second clip
                secondPart->setStartTime(RationalTime{framePosition, startTime.timescale});
                secondPart->setDuration(RationalTime{secondDuration, startTime.timescale});
                
                // Insert second clip after original
                track.clips.insert(track.clips.begin() + clipIndex + 1, secondPart);
                
                return secondPart;
            }
        }
    }
    return nullptr;
}

ClipPtr Timeline::splitAudioClip(int trackId, int clipIndex, int64_t framePosition) {
    // Similar implementation to splitVideoClip
    return splitVideoClip(trackId, clipIndex, framePosition);
}

std::vector<ClipPtr> Timeline::getClipsAtFrame(int64_t frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ClipPtr> result;
    
    // Check video tracks (return first visible clip from topmost track)
    for (auto it = videoTracks_.rbegin(); it != videoTracks_.rend(); ++it) {
        if (it->muted) continue;
        
        for (const auto& clip : it->clips) {
            auto startTime = clip->getStartTime().value;
            auto endTime = clip->getEndTime().value;
            
            if (frame >= startTime && frame < endTime) {
                result.push_back(clip);
                break; // Only one clip per track at a time
            }
        }
    }
    
    // Check audio tracks
    for (auto it = audioTracks_.rbegin(); it != audioTracks_.rend(); ++it) {
        if (it->muted) continue;
        
        for (const auto& clip : it->clips) {
            auto startTime = clip->getStartTime().value;
            auto endTime = clip->getEndTime().value;
            
            if (frame >= startTime && frame < endTime) {
                result.push_back(clip);
                break;
            }
        }
    }
    
    return result;
}

void Timeline::recalculateDuration() {
    int64_t maxFrame = 0;
    
    for (const auto& track : videoTracks_) {
        for (const auto& clip : track.clips) {
            int64_t endFrame = clip->getEndTime().value;
            if (endFrame > maxFrame) {
                maxFrame = endFrame;
            }
        }
    }
    
    for (const auto& track : audioTracks_) {
        for (const auto& clip : track.clips) {
            int64_t endFrame = clip->getEndTime().value;
            if (endFrame > maxFrame) {
                maxFrame = endFrame;
            }
        }
    }
    
    totalDuration_.value = maxFrame;
    totalDuration_.timescale = static_cast<int64_t>(fps_);
}

} // namespace IronCut
