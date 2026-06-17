#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <chrono>
#include "timeline.h"
#include "engine.h"
#include "clip.h"

using namespace IronCut;

void printHelp() {
    std::cout << "IronCut - Professional Video Editor\n";
    std::cout << "====================================\n\n";
    std::cout << "Usage: ironcut [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help              Show this help message\n";
    std::cout << "  --version           Show version information\n";
    std::cout << "  --create-project    Create a new project\n";
    std::cout << "  --export <file>     Export timeline to file\n";
    std::cout << "\n";
    std::cout << "Interactive Commands:\n";
    std::cout << "  add-video <path>    Add video clip\n";
    std::cout << "  add-audio <path>    Add audio clip\n";
    std::cout << "  play                Start playback\n";
    std::cout << "  pause               Pause playback\n";
    std::cout << "  stop                Stop playback\n";
    std::cout << "  seek <frame>        Seek to frame\n";
    std::cout << "  export <path>       Export video\n";
    std::cout << "  quit                Exit application\n";
}

void printVersion() {
    std::cout << "IronCut v1.0.0\n";
    std::cout << "Professional C++ Video Editing Engine\n";
    std::cout << "Built with FFmpeg\n";
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (argc > 1) {
        std::string arg = argv[1];
        
        if (arg == "--help" || arg == "-h") {
            printHelp();
            return 0;
        }
        
        if (arg == "--version" || arg == "-v") {
            printVersion();
            return 0;
        }
    }
    
    std::cout << "=================================\n";
    std::cout << "  IronCut Video Editor v1.0.0\n";
    std::cout << "=================================\n\n";
    std::cout << "Type 'help' for available commands\n\n";
    
    // Create timeline (1920x1080 at 30fps)
    auto timeline = std::make_shared<Timeline>(1920, 1080, 30.0);
    
    // Create engine
    auto engine = std::make_shared<Engine>();
    
    if (!engine->initialize(timeline)) {
        std::cerr << "Failed to initialize engine: " << engine->getLastError() << std::endl;
        return 1;
    }
    
    // Set up progress callback
    engine->setProgressCallback([](float progress, const std::string& status) {
        int percent = static_cast<int>(progress * 100);
        std::cout << "\rExport Progress: [";
        int barWidth = 50;
        int pos = static_cast<int>(barWidth * progress);
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << percent << "% - " << status << std::flush;
        if (progress >= 1.0f) std::cout << std::endl;
    });
    
    // Interactive loop
    std::string command;
    bool running = true;
    
    while (running) {
        std::cout << "\nironcut> ";
        std::cin >> command;
        
        if (command == "quit" || command == "exit" || command == "q") {
            running = false;
        }
        else if (command == "help" || command == "h" || command == "?") {
            std::cout << "\nAvailable commands:\n";
            std::cout << "  add-video <path>    - Add video clip to timeline\n";
            std::cout << "  add-audio <path>    - Add audio clip to timeline\n";
            std::cout << "  play                - Start playback\n";
            std::cout << "  pause               - Pause playback\n";
            std::cout << "  stop                - Stop playback\n";
            std::cout << "  seek <frame>        - Seek to specific frame\n";
            std::cout << "  status              - Show timeline status\n";
            std::cout << "  export <path>       - Export video to file\n";
            std::cout << "  help                - Show this help\n";
            std::cout << "  quit                - Exit application\n";
        }
        else if (command == "add-video") {
            std::string path;
            std::cin >> path;
            
            auto clip = std::make_shared<Clip>(path, MediaType::VIDEO);
            int trackId = timeline->addVideoTrack();
            
            if (timeline->addClipToVideoTrack(trackId, clip, 0)) {
                std::cout << "Video added to track " << trackId << std::endl;
            } else {
                std::cout << "Failed to add video" << std::endl;
            }
        }
        else if (command == "add-audio") {
            std::string path;
            std::cin >> path;
            
            auto clip = std::make_shared<Clip>(path, MediaType::AUDIO);
            int trackId = timeline->addAudioTrack();
            
            if (timeline->addClipToAudioTrack(trackId, clip, 0)) {
                std::cout << "Audio added to track " << trackId << std::endl;
            } else {
                std::cout << "Failed to add audio" << std::endl;
            }
        }
        else if (command == "play") {
            if (engine->play()) {
                std::cout << "Playback started" << std::endl;
            } else {
                std::cout << "Failed to start playback" << std::endl;
            }
        }
        else if (command == "pause") {
            engine->pause();
            std::cout << "Playback paused" << std::endl;
        }
        else if (command == "stop") {
            engine->stop();
            std::cout << "Playback stopped" << std::endl;
        }
        else if (command == "seek") {
            int64_t frame;
            std::cin >> frame;
            
            if (engine->seek(frame)) {
                std::cout << "Seeked to frame " << frame << std::endl;
            } else {
                std::cout << "Invalid frame number" << std::endl;
            }
        }
        else if (command == "status") {
            std::cout << "\nTimeline Status:\n";
            std::cout << "  Resolution: " << timeline->getWidth() << "x" << timeline->getHeight() << std::endl;
            std::cout << "  FPS: " << timeline->getFPS() << std::endl;
            std::cout << "  Duration: " << timeline->getTotalDuration().seconds() << " seconds\n";
            std::cout << "  Current Frame: " << engine->getCurrentFrame() << std::endl;
            std::cout << "  Playing: " << (engine->isPlaying() ? "Yes" : "No") << std::endl;
        }
        else if (command == "export") {
            std::string outputPath;
            std::cin >> outputPath;
            
            ExportSettings settings;
            settings.outputPath = outputPath;
            settings.quality = RenderQuality::HIGH;
            settings.videoBitrate = 8000000;
            settings.audioBitrate = 192000;
            
            std::cout << "Starting export to " << outputPath << "...\n";
            
            if (engine->exportVideo(settings)) {
                std::cout << "Export started (runs in background)\n";
                
                // Wait for export to complete
                while (engine->isExporting()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                
                if (!engine->getLastError().empty()) {
                    std::cerr << "\nExport failed: " << engine->getLastError() << std::endl;
                } else {
                    std::cout << "Export completed successfully!\n";
                }
            } else {
                std::cerr << "Failed to start export: " << engine->getLastError() << std::endl;
            }
        }
        else if (command.empty()) {
            // Ignore empty input
        }
        else {
            std::cout << "Unknown command: " << command << "\n";
            std::cout << "Type 'help' for available commands\n";
        }
    }
    
    std::cout << "\nGoodbye!\n";
    
    return 0;
}
