extern "C" {
#include <X11/Xutil.h>
}
#include <cstring>
#include <algorithm>
#include "../include/window_manager.h"

using ::std::unique_ptr;
bool WindowManager::wm_detected_;
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
root_(DefaultRootWindow(display)),WM_PROTOCOLS(XInternAtom(display_, "WM_PROTOCOLS", false)),
      WM_DELETE_WINDOW(XInternAtom(display_, "WM_DELETE_WINDOW", false)) 
{
    const unsigned int default_screen = DefaultScreen(display_);
    const unsigned int screen_width = DisplayWidth(display_,default_screen);
    const unsigned int screen_height = DisplayHeight(display_,default_screen);
    const Window background = XCreateSimpleWindow(
        display_,
        root_,
        0,
        0,
        screen_width,
        screen_height,
        0,
        0x000000,
        0xfff7de);
        const Window panel = XCreateSimpleWindow(
        display_,
        root_,
        0,
        0,
        screen_width,
        (int)(screen_height*0.035),
        0,
        0x000000,
        0xffb3e2);
    XMapWindow(display_,panel);
    XMapWindow(display_,background);
}
WindowManager::~WindowManager()
{
    XCloseDisplay(display_);
}
void WindowManager::Run()
{
    wm_detected_ = false;
    XSetErrorHandler(&WindowManager::OnWmDetected);
    XSelectInput(
        display_,
        root_,
        SubstructureRedirectMask | SubstructureNotifyMask
    );
    XSync(display_,false);
    if(wm_detected_)
    {
        std::cout << "Detected wm\n";
        return;
    }
    XSetErrorHandler(&WindowManager::OnXError); 
    //event loop
    for(;;)
    {
        //Get the next event
        XEvent e;
        XNextEvent(display_,&e);
        std::cout<<"Event type : "<<e.type<<std::endl;
        switch(e.type)
        {
            case CreateNotify:
                OnCreateNotify(e.xcreatewindow);
            break;
            case DestroyNotify:
                OnDestroyNotify(e.xdestroywindow);
            break;
            case ReparentNotify:
                OnReparentNotify(e.xreparent);
            break;
            case UnmapNotify:
                OnUnmapNotify(e.xunmap);
            break;
            case FocusIn:
                std::cout << "Focus in";
            case EnterNotify:
                std::cout << "On mouse over";
                OnEnterNotify(e.xcrossing);
            case LeaveNotify:
                std::cout << "On mouse out";
                OnLeaveNotify(e.xcrossing);
            case Button1MotionMask:
                std::cout << "ATO BTN1 PRESSED\n";
                OnButton1MotionMask(e.xbutton);
            //break;
            case MotionNotify:
                OnMotionNotify(e.xmotion);
            break;
            case ConfigureRequest:
                OnConfigureRequest(e.xconfigurerequest);
            break;
            case MapRequest:
                OnMapRequest(e.xmaprequest);
            break;
            case KeyPress:
                OnKeyPress(e.xkey);
            break;
            case ButtonPressMask:
                std::cout << "ATO BTN PRESSED\n";
                OnButtonPress(e.xbutton);
            break;
            case ButtonRelease:
                OnButtonRelease(e.xbutton);
                if (e.xbutton.button == Button1){
                    std::cout<<"Realese\n";
                }

            break;
            default:
                std::cout << "Ignored event : "<<e.type<<std::endl;
        }
    }
    //Grab server to prevent window from changingunder us while we frame them
    XGrabServer(display_);
    //Frame existing top level window
    Window returned_root, returned_parent;  
    Window *top_level_window;
    unsigned int num_top_level_windows;
    if(!XQueryTree(display_,root_,&returned_root,&returned_parent,&top_level_window,&num_top_level_windows))
    {
        std::cout << "Error for grabing the server\n";
    }
    for(unsigned int i = 0;i<num_top_level_windows;i++)
    {
        Frame(top_level_window[i],true/*was created before wm*/);
    }
    XFree(top_level_window);
    XUngrabServer(display_);
}

void WindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{   

}
void WindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{
    UnFrame(e.window);
    std::cout << "Window destryed gracefully";
}
void WindowManager::OnReparentNotify(const XReparentEvent& e)
{

}
void  WindowManager::OnConfigureRequest(const XConfigureRequestEvent& e)
{
    XWindowChanges changes;
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if(clients_.count(e.window))
    {
        const Window frame = clients_[e.window];
        XConfigureWindow(display_, frame,e.value_mask,&changes);
    }
    XConfigureWindow(display_, e.window,e.value_mask,&changes);

}
void WindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    //1. Frame or reframe window
    Frame(e.window,false);
    //2. Actually map window
    XMapWindow(display_,e.window);
}
void WindowManager::OnUnmapNotify(const XUnmapEvent& e)
{
    if(!clients_.count(e.window))
    {
        std::cout<<"Ignore un mapping for non window client\n";
        return;
    }
    if(e.event == root_)
    {
        std::cout << "Ignore unmap notify for existing reparenting window" << e.window << std::endl;
        return;
    }
}
void WindowManager::OnEnterNotify (const XCrossingEvent &e)
{

}
void WindowManager::OnLeaveNotify (const XCrossingEvent &e)
{

}
void WindowManager::OnKeyPress(const XKeyEvent &e)
{
    if((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display_,XK_F4)))
    {
        Atom *supported_protocols;
        int num_supported_protocols;
        if(XGetWMProtocols(display_,e.window,&supported_protocols,&num_supported_protocols) && (::std::find(supported_protocols,supported_protocols+num_supported_protocols,WM_DELETE_WINDOW) != supported_protocols+num_supported_protocols))
        {
            std::cout<<"Gracefully delete window\n";
            //Construct message
            XEvent msg;
            memset(&msg,0,sizeof(msg));
            msg.xclient.type = ClientMessage;
            msg.xclient.message_type = WM_PROTOCOLS;
            msg.xclient.window = e.window;
            msg.xclient.format = 32;
            msg.xclient.data.l[0] = WM_DELETE_WINDOW;
            //Unframe window
            UnFrame(e.window);
            if(!XSendEvent(display_,e.window,false,0,&msg))
            {
                std::cout<<"Can't kill window\n";
                return;
            }
        }
        else
        {
            std::cout<<"Killing window \n";
            XKillClient(display_,e.window);
        }
    }
    else if((e.state & Mod1Mask) && (e.keycode == XKeysymToKeycode(display_,XK_Tab)))
    {
        //Alt tab switch window
        auto i = clients_.find(e.window);
        ++i;
        if(i == clients_.end())
        {
            i = clients_.begin();
        }
        //raise and set focus
        XRaiseWindow(display_,i->second);
        XSetInputFocus(display_,i->first,RevertToPointerRoot,CurrentTime);
    }
}
void WindowManager::OnButtonPress(const XButtonEvent &e)
{
   /* if(!clients_.count(e.window))
    {
        std::cout<<"Error on the Button pres event\n";
        return;
    }*/
    std::cout<<"On the Button pres event"<<e.window<<"\n";
   // const Window frame = clients_[e.window];
   const Window frame = e.window;
    XWindowAttributes w_attribute;
    XGetWindowAttributes(display_,frame,&w_attribute);
    //Save the initai state of cursor
    drag_start_position = Position<int>(e.x_root,e.y_root);

    //Save the initial windows info
    Window returned_root;
    int x,y;
    unsigned width,height,border_width,depth;
    if(!XGetGeometry(
        display_,
        frame,
        &returned_root,
        &x, &y,
        &width,&height,
        &border_width,
        &depth
    )){
        std::cout<<"Error while save the window state while attempting to move it\n";
        return;
    }
    //Position of the window
    std::cout<<"x : "<<x<<" == "<<"y"<<y<<"\n";
    drag_start_frame_pos = Position<int>(x,y);
    drag_start_frame_size = Size<int>(width,height);
    //3 Raise clicked window on top
    XRaiseWindow(display_,e.window);
    std::cout<< "Boutton clicked\n";
}
void WindowManager::OnButton1MotionMask(const XButtonEvent &e)
{
    //Save the initai state of cursor
    std::cout<<"On the button MotionMask\n";
    /*drag_start_position = Position<int>(e.x_root,e.y_root);
    Window returned_root;
    int x,y;
    unsigned width,height,border_width,depth;
    if(!XGetGeometry(
        display_,
        e.window,
        &returned_root,
        &x, &y,
        &width,&height,
        &border_width,
        &depth
    )){
        std::cout<<"Error while save the window state while attempting to move it\n";
        return;
    }
    drag_start_frame_pos = Position<int>(x,y);
    drag_start_frame_size = Size<int>(width,height);
    //3 Raise clicked window on top
    XRaiseWindow(display_,e.window);
    std::cout<< "Boutton clicked\n";*/
}
void WindowManager::OnMotionNotify(const XMotionEvent &e)
{
    std::cout<<"Motion notify\n";
    const Position<int> drag_pos(e.x_root,e.y_root);
    const Vector2D<int> delta = drag_pos - drag_start_position;
    std::cout<<"x :"<<e.x_root<<"y :"<<e.y_root<<"\n";
    std::cout<<"x :"<<drag_start_frame_pos.x<<"y :"<<drag_start_frame_pos.y<<"\n";
    if(e.state & Button1Mask)
    {
        //alt left : Move window
        const Position<int> dest_frame_pos = drag_start_frame_pos + delta;
        XMoveWindow(display_,e.window,dest_frame_pos.x,dest_frame_pos.y);
        std::cout<<"Attempting to change position\n";
    }
}
void WindowManager::OnButtonRelease(const XButtonEvent &e)
{
    XRaiseWindow(display_,e.window);
}
void WindowManager::UnFrame(Window w)
{
    const Window frame = clients_[w];
    //1. Unmap frame
    XUnmapWindow(display_,frame);
    //2. Reparent window back to root
    XReparentWindow(
        display_,
        w,
        root_,
        0,0
    );
    XRemoveFromSaveSet(display_,w);
    //3.Destroy frame
    XDestroyWindow(display_,frame);
    //4. Drop referance to frame handle
    clients_.erase(w);
    std::cout << "Unframed window\n";
}
void WindowManager::Frame(Window w, bool was_created_before_wm)
{
    //Visual property of the frame
    const unsigned int BORDER_WIDTH = 0;
    const unsigned int BORDER_COLOR = 0;
    const unsigned int BG_COLOR = 0x031ba3;
    const unsigned int default_screen = DefaultScreen(display_);
    const unsigned int screen_width = DisplayWidth(display_,default_screen);
    const unsigned int screen_height = DisplayHeight(display_,default_screen);
    const unsigned int frame_height = (int)(0.03f*screen_height);
    const unsigned int h0 = (int)(screen_height*0.035);
    XWindowAttributes x_window_attrs;
    if(!XGetWindowAttributes(display_, w, &x_window_attrs))
    {
        std::cout << "Window attribut null for wm\n";
        return;
    }

    //2. If created before wm, we should frame it
    if(was_created_before_wm)
    {
        if(x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable)
        {
            return;
        }
    }
    
    //3.Create Frame
    const Window frame = XCreateSimpleWindow(
        display_,
        root_,
        x_window_attrs.x,
        h0,
        x_window_attrs.width,
        x_window_attrs.height + frame_height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR);
    const Window close = XCreateSimpleWindow(
        display_,
        frame,
        x_window_attrs.width - frame_height+1,
        0,
        frame_height,
        frame_height,
        0,
        0xb5001e,
        0xb5001e
    );
    //4. Select event on frame
    XSelectInput(display_,frame, ButtonPressMask|PointerMotionMask|KeyPressMask|SubstructureRedirectMask | SubstructureNotifyMask);
    //4.1 Select event on close button
    XSelectInput(display_,close, ButtonPressMask|EnterWindowMask|LeaveWindowMask);
    //5. Ad client to save set so that it will be restoredand kepts aliveif we crash
    XAddToSaveSet(display_, w);
    //6. Reparent window
    XReparentWindow(
        display_,
        w,
        frame,
        0,frame_height
    );
    //8. Save frame handle
    clients_[w] = frame;
    //Frame window event
    XGrabPointer(
        display_,
        frame,
        false,
        Button1MotionMask,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );
    //Close button
    XGrabPointer(
        display_,
        close,
        false,
        EnterWindowMask|LeaveWindowMask|FocusIn,
        GrabModeAsync,
        GrabModeAsync,
        None,
        None,
        CurrentTime
    );
    //9. Grab event for window mangagement action
    //Move window with alt+left button
    XGrabButton(
      display_,
      Button1,
      Mod1Mask,
      w,
      false,
      ButtonPress | ButtonReleaseMask | ButtonMotionMask,
      GrabModeAsync,
      GrabModeAsync,
      None,
      None);
    //Resize window with alt+right button
    //XGrabButton();
    //Kill window with alt+f4
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_F4),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync
    );
    //Switch between tab using alt + tab
    XGrabKey(
        display_,
        XKeysymToKeycode(display_, XK_Tab),
        Mod1Mask,
        w,
        false,
        GrabModeAsync,
        GrabModeAsync
    );
    //7. Map frame
    XMapWindow(display_,frame);
    XMapWindow(display_,close);
    std::cout << "Window framed\n";

}
//Error handler
int WindowManager::OnWmDetected(Display *display, XErrorEvent *e)
{
    if(static_cast<int>(e->error_code) == BadAccess)
    {
        wm_detected_ = true;
        return EXIT_FAILURE;
    }
    return 0;
}
int WindowManager::OnXError(Display *display, XErrorEvent *e)
{
    return 0;
}