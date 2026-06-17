#include "clip.h"
#include <algorithm>

namespace IronCut {

Clip::Clip(const std::string& filePath, MediaType type)
    : filePath_(filePath)
    , type_(type)
    , duration_{0, 30}  // Default: 0 frames at 30fps
    , startTime_{0, 30}
    , endTime_{0, 30}
    , volume_(1.0f)
    , muted_(false)
{
}

Clip::~Clip() {
}

void Clip::calculateEndTime() {
    if (duration_.timescale == startTime_.timescale) {
        endTime_.value = startTime_.value + duration_.value;
        endTime_.timescale = startTime_.timescale;
    } else {
        // Convert to common timescale
        double startSec = startTime_.seconds();
        double durationSec = duration_.seconds();
        endTime_.value = static_cast<int64_t>((startSec + durationSec) * endTime_.timescale);
    }
}

void Clip::trimIn(int64_t frames) {
    if (frames <= 0) return;
    
    // Reduce duration by frames
    if (duration_.value > frames) {
        duration_.value -= frames;
        
        // Adjust start time in source (not implemented here, would need sourceTime_)
        calculateEndTime();
    } else {
        // Clip would be empty
        duration_.value = 0;
        calculateEndTime();
    }
}

void Clip::trimOut(int64_t frames) {
    if (frames <= 0) return;
    
    // Reduce duration by frames
    if (duration_.value > frames) {
        duration_.value -= frames;
        calculateEndTime();
    } else {
        duration_.value = 0;
        calculateEndTime();
    }
}

} // namespace IronCut
