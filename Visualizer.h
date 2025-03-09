#pragma once
#include <vector>
#include <string>
#include <atomic>
#include <windows.h>
#include "AudioCapture.h"

enum class VisualizationMode
{
    WAVES,
    BARS
};

class Visualizer
{
public:
    Visualizer();
    ~Visualizer();

    bool Initialize();
    void Run();
    void Stop();

private:
    // Audio processing parameters
    static constexpr float MIN_AMPLITUDE = 0.2f;  // Minimum sound level
    static constexpr float MAX_AMPLITUDE = 35.0f; // Maximum visualization height
    static constexpr float NOISE_FACTOR = 0.1f;   // Randomization for smoother visuals

    // Display settings
    static constexpr int FREQ_BANDS = 20; // Number of frequency bands
    static constexpr int RENDER_FPS = 60; // Target framerate
    static constexpr int FRAME_MS = 1000 / RENDER_FPS;
    static constexpr int INPUT_POLL_MS = 50; // Input checking interval

    void InitializeConsole();
    void ProcessAudioData();
    std::vector<std::string> CreateHeaderBuffer() const;
    void WriteToConsole(const std::vector<std::string> &buffer);
    void HandleInput();
    void RenderFrame();
    void GetConsoleSize(int &width, int &height) const;

    // Renders audio data using specified visualization character
    template <typename T>
    std::vector<std::string> RenderVisualization(const T &style) const
    {
        std::vector<std::string> buffer(consoleHeight);
        std::string emptyLine(consoleWidth, ' ');

        // Initialize buffer with empty lines
        for (int i = 0; i < consoleHeight; i++)
        {
            buffer[i] = emptyLine;
        }

        int barWidth = consoleWidth / frequencyData.size();

        // Draw visualization
        for (size_t i = 0; i < frequencyData.size(); ++i)
        {
            int barHeight = static_cast<int>(frequencyData[i]);
            for (int h = 0; h < barHeight && h < consoleHeight; ++h)
            {
                for (int w = i * barWidth; w < (i + 1) * barWidth && w < consoleWidth; ++w)
                {
                    if constexpr (std::is_same_v<T, std::string>)
                    {
                        size_t charIndex = (h * style.length()) / std::max(1, barHeight);
                        buffer[consoleHeight - 1 - h][w] = style[charIndex];
                    }
                    else
                    {
                        buffer[consoleHeight - 1 - h][w] = style;
                    }
                }
            }
        }

        return buffer;
    }

    AudioCapture audioCapture;
    std::atomic<bool> running;
    std::atomic<VisualizationMode> currentMode;
    std::vector<float> frequencyData;

    HANDLE consoleHandle;
    int consoleWidth;
    int consoleHeight;
};