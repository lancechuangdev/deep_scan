#ifndef DRAW_WINDOW_H
#define DRAW_WINDOW_H

#include <gtkmm.h>

// DrawWindow class that inherits from Gtk::Window
class DrawWindow : public Gtk::Window {
public:
    // Constructor
    DrawWindow();

protected:
    // Custom DrawingArea class that inherits from Gtk::DrawingArea
    class DrawingArea : public Gtk::DrawingArea {
    public:
        DrawingArea(); // Constructor

    protected:
        // Override the on_draw method to customize drawing
        bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
    };

    // Member variables
    DrawingArea drawing_area;
};

#endif // DRAW_WINDOW_H