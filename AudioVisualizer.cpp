#include "Visualizer.h"
#include <iostream>
#include <conio.h>

int main()
{
    std::cout << "Starting Audio Visualizer...\n";

    Visualizer visualizer;
    if (!visualizer.Initialize())
    {
        std::cout << "Press any key to exit...";
        _getch();
        return 1;
    }

    visualizer.Run();
    return 0;
}