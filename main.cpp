#include<iostream>
#include<memory>
#include<fstream>
#include "include/window_manager.h"

using ::std::unique_ptr;
int main()
{
    std::ofstream output;
    output.open("logs/output.log");
    unique_ptr<WindowManager> wm = WindowManager::Create();
    if(!wm)
    {
        output << "Failed to initialize window\n";
    }
    else
    {
        output << "Initialization success !!\n";
        wm->Run();
    }
    output.close();
    return 0;
}