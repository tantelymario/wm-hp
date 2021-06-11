#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H
extern "C" {
    #include<X11/Xlib.h>
}
#include<memory>
#include<fstream>

class WindowManager
{
public:
    static ::std::unique_ptr<WindowManager> Create();
    //Disconnect to X Server
    ~WindowManager();
    //Main loop
    void Run();
private:
    WindowManager(Display *display);
    Display *display_;
    const Window root_;
    std::ofstream *output;
};
#endif