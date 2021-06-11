#include "../include/window_manager.h"

using ::std::unique_ptr;

unique_ptr<WindowManager> WindowManager::Create()
{
    Display *display = XOpenDisplay(nullptr);
    std::ofstream output;
    output.open("logs/output.log");
    if(display == nullptr)
    {
        output << "Failed to open X display\n";
        output.close();
        return nullptr;
    }
    output.close();
    return unique_ptr<WindowManager>(new WindowManager(display));
}
WindowManager::WindowManager(Display *display):display_(display),
root_(DefaultRootWindow(display))
{
    
}
WindowManager::~WindowManager()
{
    XCloseDisplay(display_);
}
void WindowManager::Run()
{

}
