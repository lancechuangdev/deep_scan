#include "drawwindow.h"

// Implementation of DrawWindow constructor
DrawWindow::DrawWindow() {
    set_title("Draw Window");
    set_default_size(800, 600);  // Set initial size
    set_resizable(true);         // Allow resizing
    set_position(Gtk::WIN_POS_CENTER);  // Center the window

    // Pack the drawing area into the window
    add(drawing_area);
    drawing_area.show();
}

// Implementation of DrawingArea constructor
DrawWindow::DrawingArea::DrawingArea() {
    // Additional setup for the drawing area can be done here
}

// Override on_draw method to customize the drawing
bool DrawWindow::DrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {
    cr->set_source_rgb(0.0, 0.0, 0.0); // Black background
    cr->paint(); // Fill the background with black
    return true;
}