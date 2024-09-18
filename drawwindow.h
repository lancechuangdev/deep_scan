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
    // Load the image buffer to the drawing area
    void loadImageBuffer(const std::string& filename);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_imageLabelingPath;
    std::vector<std::string> m_imageLabelingQueue;
    Glib::RefPtr<Gdk::Pixbuf> m_currentPixbuf; // Store the currently loaded pixbuf
};

#endif