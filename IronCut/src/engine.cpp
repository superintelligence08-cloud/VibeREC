#include "engine.h"
#include <thread>
#include <chrono>
#include <cstring>

namespace IronCut {

Engine::Engine()
    : inputFormatCtx_(nullptr)
    , videoDecoderCtx_(nullptr)
    , audioDecoderCtx_(nullptr)
    , swsCtx_(nullptr)
    , isPlaying_(false)
    , currentFrame_(0)
    , isExporting_(false)
    , exportProgress_(0.0f)
    , frameBuffer_(nullptr)
    , frameWidth_(0)
    , frameHeight_(0)
{
}

Engine::~Engine() {
    cleanup();
}

bool Engine::initialize(TimelinePtr timeline) {
    timeline_ = timeline;
    
    if (!timeline_) {
        lastError_ = "Invalid timeline";
        return false;
    }
    
    // Initialize frame buffer
    frameWidth_ = timeline_->getWidth();
    frameHeight_ = timeline_->getHeight();
    
    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frameWidth_, frameHeight_, 1);
    if (bufferSize < 0) {
        lastError_ = "Failed to calculate frame buffer size";
        return false;
    }
    
    frameBuffer_ = static_cast<uint8_t*>(av_malloc(bufferSize));
    if (!frameBuffer_) {
        lastError_ = "Failed to allocate frame buffer";
        return false;
    }
    
    return true;
}

bool Engine::play() {
    if (isPlaying_) return true;
    
    isPlaying_ = true;
    
    // Start playback thread
    std::thread([this]() {
        double fps = timeline_->getFPS();
        auto frameDuration = std::chrono::duration<double>(1.0 / fps);
        
        while (isPlaying_) {
            auto frameStart = std::chrono::steady_clock::now();
            
            // Decode and process next frame
            // In a full implementation, this would:
            // 1. Get clips at current frame from timeline
            // 2. Decode source frames
            // 3. Composite/render to frameBuffer_
            
            currentFrame_++;
            
            // Check if we've reached end of timeline
            if (currentFrame_ >= timeline_->getTotalDuration().value) {
                stop();
                break;
            }
            
            // Maintain frame rate
            auto frameEnd = std::chrono::steady_clock::now();
            auto elapsed = frameEnd - frameStart;
            
            if (elapsed < frameDuration) {
                std::this_thread::sleep_for(frameDuration - elapsed);
            }
        }
    }).detach();
    
    return true;
}

bool Engine::pause() {
    isPlaying_ = false;
    return true;
}

bool Engine::stop() {
    isPlaying_ = false;
    currentFrame_ = 0;
    return true;
}

bool Engine::seek(int64_t frame) {
    if (frame < 0 || frame >= timeline_->getTotalDuration().value) {
        return false;
    }
    
    currentFrame_ = frame;
    
    // In a full implementation, seek all active decoders
    // to the appropriate position in their source files
    
    return true;
}

uint8_t* Engine::getFrameRGB(int& width, int& height) {
    width = frameWidth_;
    height = frameHeight_;
    return frameBuffer_;
}

bool Engine::exportVideo(const ExportSettings& settings) {
    if (isExporting_) {
        lastError_ = "Export already in progress";
        return false;
    }
    
    if (!timeline_) {
        lastError_ = "No timeline loaded";
        return false;
    }
    
    isExporting_ = true;
    exportProgress_ = 0.0f;
    
    // Run export in background thread
    std::thread([this, settings]() {
        AVFormatContext* outFmtCtx = nullptr;
        AVCodecContext* encCtx = nullptr;
        
        // Create output format context
        avformat_alloc_output_context2(&outFmtCtx, nullptr, nullptr, settings.outputPath.c_str());
        if (!outFmtCtx) {
            lastError_ = "Failed to create output format context";
            isExporting_ = false;
            return;
        }
        
        // Find encoder
        const AVCodec* codec = avcodec_find_encoder_by_name(settings.videoCodec.c_str());
        if (!codec) {
            lastError_ = "Encoder not found: " + settings.videoCodec;
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        // Create encoder context
        encCtx = avcodec_alloc_context3(codec);
        if (!encCtx) {
            lastError_ = "Failed to allocate encoder context";
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        // Set encoder parameters
        encCtx->bit_rate = settings.videoBitrate;
        encCtx->width = timeline_->getWidth();
        encCtx->height = timeline_->getHeight();
        encCtx->time_base = AVRational{1, static_cast<int>(timeline_->getFPS())};
        encCtx->framerate = AVRational{static_cast<int>(timeline_->getFPS()), 1};
        encCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        encCtx->gop_size = settings.keyframeInterval;
        
        // Open encoder
        if (avcodec_open2(encCtx, codec, nullptr) < 0) {
            lastError_ = "Failed to open encoder";
            avcodec_free_context(&encCtx);
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        // Add video stream to output
        AVStream* outStream = avformat_new_stream(outFmtCtx, nullptr);
        if (!outStream) {
            lastError_ = "Failed to create output stream";
            avcodec_free_context(&encCtx);
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        avcodec_parameters_from_context(outStream->codecpar, encCtx);
        
        // Open output file
        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&outFmtCtx->pb, settings.outputPath.c_str(), AVIO_FLAG_WRITE) < 0) {
                lastError_ = "Failed to open output file";
                avcodec_free_context(&encCtx);
                avformat_free_context(outFmtCtx);
                isExporting_ = false;
                return;
            }
        }
        
        // Write file header
        if (avformat_write_header(outFmtCtx, nullptr) < 0) {
            lastError_ = "Failed to write file header";
            avio_closep(&outFmtCtx->pb);
            avcodec_free_context(&encCtx);
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        // Encoding loop
        int64_t totalFrames = timeline_->getTotalDuration().value;
        AVFrame* frame = av_frame_alloc();
        AVPacket* pkt = av_packet_alloc();
        
        if (!frame || !pkt) {
            lastError_ = "Failed to allocate frame or packet";
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avio_closep(&outFmtCtx->pb);
            avcodec_free_context(&encCtx);
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        frame->format = encCtx->pix_fmt;
        frame->width = encCtx->width;
        frame->height = encCtx->height;
        
        if (av_frame_get_buffer(frame, 0) < 0) {
            lastError_ = "Failed to allocate frame buffer";
            av_frame_free(&frame);
            av_packet_free(&pkt);
            avio_closep(&outFmtCtx->pb);
            avcodec_free_context(&encCtx);
            avformat_free_context(outFmtCtx);
            isExporting_ = false;
            return;
        }
        
        // Process each frame
        for (int64_t i = 0; i < totalFrames && isExporting_; i++) {
            // Update progress
            exportProgress_ = static_cast<float>(i) / totalFrames;
            
            if (progressCallback_) {
                progressCallback_(exportProgress_, "Encoding frame " + std::to_string(i) + "/" + std::to_string(totalFrames));
            }
            
            // In a full implementation:
            // 1. Get clips at frame i from timeline
            // 2. Decode/composite frame into 'frame'
            
            // Dummy frame generation (replace with actual rendering)
            for (int y = 0; y < frame->height; y++) {
                uint8_t* dst = frame->data[0] + y * frame->linesize[0];
                for (int x = 0; x < frame->width; x++) {
                    int offset = y * frame->linesize[0] + x;
                    // Create gradient pattern
                    dst[x * 3 + 0] = (x * 255) / frame->width;   // R
                    dst[x * 3 + 1] = (y * 255) / frame->height;  // G
                    dst[x * 3 + 2] = 128;                         // B
                }
            }
            
            frame->pts = i;
            
            // Send frame to encoder
            if (avcodec_send_frame(encCtx, frame) < 0) {
                lastError_ = "Failed to send frame to encoder";
                break;
            }
            
            // Receive packets
            while (avcodec_receive_packet(encCtx, pkt) == 0) {
                pkt->stream_index = 0;
                av_packet_rescale_ts(pkt, encCtx->time_base, outStream->time_base);
                
                if (av_interleaved_write_frame(outFmtCtx, pkt) < 0) {
                    lastError_ = "Failed to write packet";
                    break;
                }
                
                av_packet_unref(pkt);
            }
        }
        
        // Flush encoder
        avcodec_send_frame(encCtx, nullptr);
        while (avcodec_receive_packet(encCtx, pkt) == 0) {
            pkt->stream_index = 0;
            av_packet_rescale_ts(pkt, encCtx->time_base, outStream->time_base);
            av_interleaved_write_frame(outFmtCtx, pkt);
            av_packet_unref(pkt);
        }
        
        // Write trailer
        av_write_trailer(outFmtCtx);
        
        // Cleanup
        av_frame_free(&frame);
        av_packet_free(&pkt);
        
        if (!(outFmtCtx->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&outFmtCtx->pb);
        }
        
        avcodec_free_context(&encCtx);
        avformat_free_context(outFmtCtx);
        
        exportProgress_ = 1.0f;
        isExporting_ = false;
        
        if (progressCallback_) {
            progressCallback_(1.0f, "Export complete!");
        }
        
    }).detach();
    
    return true;
}

void Engine::cancelExport() {
    isExporting_ = false;
}

bool Engine::openMediaFile(const std::string& filePath) {
    // Implementation for opening media files
    return true;
}

bool Engine::decodeNextFrame() {
    // Implementation for decoding next frame
    return true;
}

bool Engine::writeOutputFrame(AVFrame* frame, AVFormatContext* outFmtCtx, AVCodecContext* encCtx) {
    // Implementation for writing encoded frame
    return true;
}

void Engine::cleanup() {
    if (frameBuffer_) {
        av_free(frameBuffer_);
        frameBuffer_ = nullptr;
    }
    
    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
    
    if (videoDecoderCtx_) {
        avcodec_free_context(&videoDecoderCtx_);
    }
    
    if (audioDecoderCtx_) {
        avcodec_free_context(&audioDecoderCtx_);
    }
    
    if (inputFormatCtx_) {
        avformat_close_input(&inputFormatCtx_);
    }
}

} // namespace IronCut
