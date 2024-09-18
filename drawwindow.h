#ifndef DRAWWINDOW_H
#define DRAWWINDOW_H

#include <iostream>
#include <gtkmm.h>

class DrawWindow : public Gtk::Window {
public:
    DrawWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& refGlade);
    static DrawWindow* create(const std::string &gladeFailePath);
    void setImageLabelingPath(const std::string &path);

protected:
    Gtk::DrawingArea *m_drawingArea;

    void on_window_shown();
    bool onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context>& cr);
    void loadImageBuffer(const std::string& filename); // Load the image buffer to the drawing area

    // Mouse event
    bool onScrollEvent(GdkEventScroll* scroll_event);
    bool onButtonPressEvent(GdkEventButton* button_event);
    bool onButtonReleaseEvent(GdkEventButton* button_event);
    bool onMotionNotifyEvent(GdkEventMotion* motion_event);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_imageLabelingPath;
    std::vector<std::string> m_imageLabelingQueue;
    Glib::RefPtr<Gdk::Pixbuf> m_currentPixbuf; // Store the currently loaded pixbuf

    double m_zoomFactor = 1.0;    // Zoom factor (1.0 = no zoom)
    double m_offsetX = 0.0;       // Horizontal pan offset
    double m_offsetY = 0.0;       // Vertical pan offset
    double m_dragStartX = 0.0;    // Mouse drag start X
    double m_dragStartY = 0.0;    // Mouse drag start Y
    bool m_isDragging = false;    // Track whether the user is dragging
};

#endif