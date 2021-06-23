#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H
extern "C" {
    #include<X11/Xlib.h>
}
#include<memory>
#include<iostream>
#include <ostream>
#include <string>
#include<fstream>
#include<unordered_map>

template <typename T>
struct Position
{
    T x,y;
    Position() = default;
    Position(T _x, T _y)
    {
        x = _x;
        y = _y;
    };
};
// Represents a 2D size.
template <typename T>
struct Size {
  T width, height;

  Size() = default;
  Size(T w, T h)
      : width(w), height(h) {
  }

  ::std::string ToString() const;
};
// Represents a 2D vector.
template <typename T>
struct Vector2D {
  T x, y;

  Vector2D() = default;
  Vector2D(T _x, T _y)
      : x(_x), y(_y) {
  }
};
// Position operators.
// Outputs a Size<T> as a string to a std::ostream.
template <typename T>
::std::ostream& operator << (::std::ostream& out, const Position<T>& pos);
template <typename T>
Vector2D<T> operator - (const Position<T>& a, const Position<T>& b);
template <typename T>
Position<T> operator + (const Position<T>& a, const Vector2D<T> &v);
template <typename T>
Position<T> operator + (const Vector2D<T> &v, const Position<T>& a);
template <typename T>
Position<T> operator - (const Position<T>& a, const Vector2D<T> &v);
template <typename T>
Vector2D<T> operator - (const Position<T>& a, const Position<T>& b) {
  return Vector2D<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
Position<T> operator + (const Position<T>& a, const Vector2D<T> &v) {
  return Position<T>(a.x + v.x, a.y + v.y);
}

template <typename T>
Position<T> operator + (const Vector2D<T> &v, const Position<T>& a) {
  return Position<T>(a.x + v.x, a.y + v.y);
}

template <typename T>
Position<T> operator - (const Position<T>& a, const Vector2D<T> &v) {
  return Position<T>(a.x - v.x, a.y - v.y);
}

template <typename T>
Vector2D<T> operator - (const Size<T>& a, const Size<T>& b) {
  return Vector2D<T>(a.width - b.width, a.height - b.height);
}

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
    //XLib error handler
    static int OnXError(Display * display, XErrorEvent *e);
    static int OnWmDetected(Display *display, XErrorEvent *e);
    void OnCreateNotify(const XCreateWindowEvent& e);
    void OnDestroyNotify(const XDestroyWindowEvent& e);
    void OnUnmapNotify(const XUnmapEvent& e);
    void OnReparentNotify(const XReparentEvent& e);
    void OnMotionNotify(const XMotionEvent &e);
    void OnConfigureRequest(const XConfigureRequestEvent& e);
    void OnMapRequest(const XMapRequestEvent& e);
    void OnKeyPress(const XKeyEvent& e);
    void OnButtonPress(const XButtonEvent& e);
    void OnButton1MotionMask(const XButtonEvent& e);
    void OnEnterNotify(const XEnterWindowEvent& e);
    void OnLeaveNotify(const XLeaveWindowEvent& e);
    void OnButtonRelease(const XButtonEvent& e);
    void Frame(Window w, bool was_created_before_wm);
    void UnFrame(Window w);
    Display *display_;
    const Window root_;
    std::ofstream *output;
    static bool wm_detected_;
    ::std::unordered_map<Window,Window> clients_;
    // Atom constants.
    const Atom WM_PROTOCOLS;
    const Atom WM_DELETE_WINDOW;
    //cursor position at the start of a winoow move/resize
    Position<int> drag_start_position;
    //The position of the affected window move/resize
    Position<int> drag_start_frame_pos;
    //the size of the affected window move/resize
    Size<int> drag_start_frame_size;
};
#endif