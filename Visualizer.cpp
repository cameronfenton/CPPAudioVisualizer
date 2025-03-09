/*
 * CPP Audio Visualizer - Console-based audio visualization
 * Written by Cameron Fenton
 *
 * A tool to visualize audio output in real-time using
 * the Windows console (Linux support coming in a future commit). Features bar and wave visualizations.
 */
#include "Visualizer.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <conio.h>
#include <cmath>
#include <algorithm>

Visualizer::Visualizer()
{
    running = false;
    currentMode = VisualizationMode::BARS;
    consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    consoleWidth = 0;
    consoleHeight = 0;
}

Visualizer::~Visualizer()
{
    Stop();
}

bool Visualizer::Initialize()
{
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        std::cerr << "Failed to start audio system!\n";
        std::cerr << "Try to check if your audio devices are connected.\n";
        return false;
    }

    if (!audioCapture.Initialize())
    {
        std::cerr << "Couldn't access your audio device!\n";
        std::cerr << "Try checking your default audio device settings.\n";
        CoUninitialize();
        return false;
    }

    InitializeConsole();
    return true;
}

void Visualizer::InitializeConsole()
{
    // Force UTF-8 code page
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Enable virtual terminal processing and UTF-8
    DWORD consoleMode;
    GetConsoleMode(consoleHandle, &consoleMode);
    SetConsoleMode(consoleHandle,
                   consoleMode |
                       ENABLE_VIRTUAL_TERMINAL_PROCESSING |
                       ENABLE_PROCESSED_OUTPUT);

    HWND consoleWindow = GetConsoleWindow();
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(consoleHandle, &csbi);

    COORD bufferSize = {
        static_cast<SHORT>(csbi.srWindow.Right - csbi.srWindow.Left + 1),
        static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1)};

    SetConsoleScreenBufferSize(consoleHandle, bufferSize);

    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(consoleHandle, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(consoleHandle, &cursorInfo);

    consoleWidth = bufferSize.X;
    consoleHeight = bufferSize.Y;
}

void Visualizer::GetConsoleSize(int &width, int &height) const
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(consoleHandle, &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

void Visualizer::WriteToConsole(const std::vector<std::string> &buffer)
{
    COORD topLeft = {0, 0};
    DWORD written;

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(consoleHandle, &csbi);
    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(consoleHandle, ' ', length, topLeft, &written);
    FillConsoleOutputAttribute(consoleHandle, csbi.wAttributes, length, topLeft, &written);

    SetConsoleCursorPosition(consoleHandle, topLeft);

    std::string output;
    size_t maxLines = static_cast<size_t>(csbi.dwSize.Y);
    for (size_t i = 0; i < buffer.size() && i < maxLines; ++i)
    {
        output += buffer[i].substr(0, csbi.dwSize.X) + "\n";
    }

    WriteConsoleA(consoleHandle, output.c_str(), output.length(), &written, nullptr);
}

std::vector<std::string> Visualizer::CreateHeaderBuffer() const
{
    std::vector<std::string> header(4);
    header[0] = "Audio Visualizer v1.0";
    header[1] = "Controls: [B]ars | [W]aves | [R]andom Mode | [Q]uit";
    header[2] = "Random Mode: " + std::string(audioCapture.IsUsingRandomFallback() ? "ON" : "OFF");
    header[3] = "";
    return header;
}

void Visualizer::ProcessAudioData()
{
    if (!audioCapture.GetAudioData(frequencyData))
    {
        frequencyData.clear();
        if (audioCapture.IsUsingRandomFallback())
        {
            // Use random data if fallback is enabled
            for (int i = 0; i < 20; ++i)
            {
                frequencyData.push_back(static_cast<float>(rand() % 15 + 1));
            }
        }
        else
        {
            for (int i = 0; i < 20; ++i)
            {
                frequencyData.push_back(0.2f);
            }
        }
    }
    else
    {
        if (audioCapture.IsUsingRandomFallback())
        {
            audioCapture.SetRandomFallback(false);
        }

        float peakAmplitude = 0.0f;
        for (const auto &amplitude : frequencyData)
        {
            peakAmplitude = std::max<float>(peakAmplitude, amplitude);
        }

        for (auto &amplitude : frequencyData)
        {
            float normalizedLevel = amplitude / (peakAmplitude + 0.0001f);
            amplitude = MIN_AMPLITUDE + (std::pow(normalizedLevel, 0.8f) * MAX_AMPLITUDE);

            // Randomization to make it kinda look more natural
            if (amplitude > 1.0f)
            {
                float randomFactor = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * NOISE_FACTOR;
                amplitude *= (1.0f + randomFactor);
            }

            amplitude = std::clamp(amplitude, MIN_AMPLITUDE, MAX_AMPLITUDE);
        }
    }
}

void Visualizer::HandleInput()
{
    while (running)
    {
        if (_kbhit())
        {
            char key = _getch();
            switch (key)
            {
            case 'w':
            case 'W':
                currentMode = VisualizationMode::WAVES;
                break;
            case 'b':
            case 'B':
                currentMode = VisualizationMode::BARS;
                break;
            case 'r':
            case 'R':
                audioCapture.SetRandomFallback(!audioCapture.IsUsingRandomFallback());
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void Visualizer::RenderFrame()
{
    while (running)
    {
        try
        {
            ProcessAudioData();
            GetConsoleSize(consoleWidth, consoleHeight);

            std::vector<std::string> buffer = CreateHeaderBuffer();

            buffer.push_back("Status: " + std::string(frequencyData[0] > 0.2f ? "Audio detected" : "No audio"));
            buffer.push_back("");

            auto visualization = RenderVisualization(
                currentMode == VisualizationMode::WAVES ? std::string(1, '~') : std::string(1, '|'));

            size_t headerSize = buffer.size();
            size_t maxLines = (consoleHeight > headerSize) ? consoleHeight - headerSize : 0;
            size_t linesToAdd = (visualization.size() < maxLines) ? visualization.size() : maxLines;

            for (size_t i = 0; i < linesToAdd; ++i)
            {
                buffer.push_back(visualization[i]);
            }

            WriteToConsole(buffer);
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error in visualization: " << e.what() << std::endl;
            running = false;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

void Visualizer::Run()
{
    running = true;

    std::thread inputThread(&Visualizer::HandleInput, this);
    std::thread renderThread(&Visualizer::RenderFrame, this);

    if (inputThread.joinable())
        inputThread.join();
    if (renderThread.joinable())
        renderThread.join();

    CoUninitialize();
}

void Visualizer::Stop()
{
    running = false;
}