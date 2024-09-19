#include "drawwindow.h"

// Get a list of image files in the selected folder
std::vector<std::string> get_image_files_in_folder(const std::string &folder_path)
{
    std::vector<std::string> image_files;
    Glib::Dir dir(folder_path);

    // Supported image file extensions
    std::vector<std::string> image_extensions = {".jpg", ".jpeg", ".png", ".bmp"};

    // Iterate through files in the folder
    for (const auto &file : dir)
    {
        std::string file_path = folder_path + "/" + file;

        // Get the file extension by extracting the base name and finding the dot
        std::string basename = Glib::path_get_basename(file);
        std::string::size_type idx = basename.rfind('.');

        if (idx != std::string::npos)
        {
            std::string extension = basename.substr(idx); // Extract extension
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            // Check if the extension matches a supported image format
            if (std::find(image_extensions.begin(), image_extensions.end(), extension) != image_extensions.end())
            {
                image_files.push_back(file_path);
            }
        }
    }

    return image_files;
}

DrawWindow::DrawWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &DrawWindow::on_window_shown));

    m_refGlade->get_widget("mask_drawing_area", m_drawingArea);
    if (m_drawingArea)
    {
        m_drawingArea->signal_draw().connect(sigc::mem_fun(*this, &DrawWindow::onDrawingAreaDraw));

        // Connect mouse scroll event
        m_drawingArea->add_events(Gdk::SCROLL_MASK);
        m_drawingArea->signal_scroll_event().connect(sigc::mem_fun(*this, &DrawWindow::onScrollEvent));

        // Connect mouse press and motion events
        m_drawingArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_drawingArea->signal_button_press_event().connect(sigc::mem_fun(*this, &DrawWindow::onButtonPressEvent));
        m_drawingArea->signal_button_release_event().connect(sigc::mem_fun(*this, &DrawWindow::onButtonReleaseEvent));
        m_drawingArea->signal_motion_notify_event().connect(sigc::mem_fun(*this, &DrawWindow::onMotionNotifyEvent));

        // Connect key press events
        m_drawingArea->add_events(Gdk::KEY_PRESS_MASK);
    }

    m_refGlade->get_widget("circle_brush_btn", m_circleBrushBtn);
    if (m_circleBrushBtn)
    {
        m_circleBrushBtn->signal_clicked().connect(sigc::mem_fun(*this, &DrawWindow::onCircleBrushClicked));
    }
}

DrawWindow *DrawWindow::create(const std::string &gladeFailePath)
{
    // Load the Glade file
    auto refBuilder = Gtk::Builder::create_from_file(gladeFailePath);

    // Get the window object from the Glade file
    DrawWindow *window = nullptr;
    refBuilder->get_widget_derived("draw_window", window);

    if (window)
    {
        window->set_title("Label Images");
    }

    return window;
}

void DrawWindow::setImageLabelingPath(const std::string &path)
{
    m_imageLabelingPath = path;
    m_imageLabelingQueue = get_image_files_in_folder(path);
}

void DrawWindow::on_window_shown()
{
    std::cout << "Window is shown!" << std::endl;
    // Load the first image buffer to drawing area
    if (!m_imageLabelingQueue.empty())
    {
        loadImageBuffer(m_imageLabelingQueue.front());
    }
}

bool DrawWindow::onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offsetX, m_offsetY);   // Apply panning offset
    cr->scale(m_zoomFactor, m_zoomFactor); // Apply zoom

    // Draw the image
    if (m_currentPixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_currentPixbuf, 0, 0);
        cr->paint();
    }

    // If there is a mask, draw it on top of the image
    if (m_maskPixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_maskPixbuf, 0, 0);
        cr->paint();
    }

    // Draw the brush cursor on top
    drawBrushCursor(cr);

    return true; // Return true to indicate the event has been handled
}

void DrawWindow::loadImageBuffer(const std::string &filename)
{
    try
    {
        m_currentPixbuf = Gdk::Pixbuf::create_from_file(filename);

        // Reset zoom and pan when a new image is loaded
        m_zoomFactor = 1.0;
        m_offsetX = 0.0;
        m_offsetY = 0.0;

        if (m_currentPixbuf)
        {
            // Get the dimensions of the pixbuf
            int width = m_currentPixbuf->get_width();
            int height = m_currentPixbuf->get_height();

            // Set the size of the drawing area if needed
            m_drawingArea->set_size_request(width, height);

            // Create a transparent mask pixbuf of the same size as the image
            m_maskPixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
            m_maskPixbuf->fill(0xffffffbe); // Initialize the mask to be fully transparent
        }

        // Trigger a redraw of the drawing area
        m_drawingArea->queue_draw();
    }
    catch (const Glib::FileError &ex)
    {
        std::cerr << "File Error: " << ex.what() << std::endl;
    }
    catch (const Gdk::PixbufError &ex)
    {
        std::cerr << "Pixbuf Error: " << ex.what() << std::endl;
    }
}

void DrawWindow::onCircleBrushClicked()
{
    m_isDrawingMode = true;
    m_showBrushCursor = true;
}

bool DrawWindow::on_key_press_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrlPressed = true;
    }
    return Gtk::Window::on_key_press_event(key_event);
}

bool DrawWindow::on_key_release_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrlPressed = false;
    }
    return Gtk::Window::on_key_release_event(key_event);
}

// Handle mouse scroll
bool DrawWindow::onScrollEvent(GdkEventScroll *scroll_event)
{
    if (m_isDrawingMode)
    {
        if (m_ctrlPressed)
        {
            // Adjust alpha when Ctrl is pressed
            if (scroll_event->direction == GDK_SCROLL_UP)
            {
                m_brushAlpha = std::min(m_brushAlpha + 0.1, 1.0); // Max alpha is 1.0
            }
            else if (scroll_event->direction == GDK_SCROLL_DOWN)
            {
                m_brushAlpha = std::max(m_brushAlpha - 0.1, 0.1); // Min alpha is 0.1
            }
        }
        else
        {
            if (scroll_event->direction == GDK_SCROLL_UP)
            {
                m_brushRadius = std::min(m_brushRadius + 1.0, 100.0); // Cap at 100
            }
            else if (scroll_event->direction == GDK_SCROLL_DOWN)
            {
                m_brushRadius = std::max(m_brushRadius - 1.0, 5.0); // Minimum size is 5
            }
        }
    }
    else
    {
        const double zoomStep = 0.1;

        if (scroll_event->direction == GDK_SCROLL_UP)
        {
            m_zoomFactor += zoomStep;
        }
        else if (scroll_event->direction == GDK_SCROLL_DOWN)
        {
            m_zoomFactor = std::max(zoomStep, m_zoomFactor - zoomStep);
        }
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();

    // Return true to indicate that the event has been handled
    return true;
}

// Handle mouse press for starting panning
bool DrawWindow::onButtonPressEvent(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        if (m_isDrawingMode)
        {
            // Start drawing
            m_isDrawing = true;
            m_showBrushCursor = false;

            drawOnMask(button_event->x, button_event->y, m_brushRadius);
            // Trigger a redraw of the drawing area
            m_drawingArea->queue_draw();
        }
        else
        {
            // Start dragging
            m_isDragging = true;
            m_dragStartX = button_event->x;
            m_dragStartY = button_event->y;
        }
    }
    return true;
}

// Handle mouse release
bool DrawWindow::onButtonReleaseEvent(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        if (m_isDrawingMode)
        {
            // Stop drawing
            m_isDrawing = false;
            m_showBrushCursor = true;
        }
        else
        {
            // Stop dragging
            m_isDragging = false;
        }
    }
    return true;
}

// Handle mouse motion
bool DrawWindow::onMotionNotifyEvent(GdkEventMotion *motion_event)
{
    if (m_isDrawingMode)
    {
        m_brushX = motion_event->x;
        m_brushY = motion_event->y;

        if (m_isDrawing)
        {
            drawOnMask(m_brushX, m_brushY, m_brushRadius);
        }
        else
        {
        }
    }
    else
    {
        if (m_isDragging)
        {
            // Calculate the distance moved
            double deltaX = motion_event->x - m_dragStartX;
            double deltaY = motion_event->y - m_dragStartY;

            // Update the panning offset
            m_offsetX += deltaX;
            m_offsetY += deltaY;

            // Update the start position for the next motion event
            m_dragStartX = motion_event->x;
            m_dragStartY = motion_event->y;
        }
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();

    return true;
}

void DrawWindow::drawBrushCursor(const Cairo::RefPtr<Cairo::Context> &cr)
{
    if (m_showBrushCursor)
    {
        cr->set_source_rgba(255, 255, 255, m_brushAlpha);
        cr->arc(m_brushX, m_brushY, m_brushRadius, 0, 2 * M_PI);
        cr->fill();
    }
}

void DrawWindow::drawOnMask(double x, double y, double brushRadius)
{
    // Create a Cairo context from the mask pixbuf's data
    auto surface = Cairo::ImageSurface::create(
        (unsigned char*)m_maskPixbuf->get_pixels(), 
        Cairo::FORMAT_ARGB32,
        m_maskPixbuf->get_width(),
        m_maskPixbuf->get_height(),
        m_maskPixbuf->get_rowstride()
    );

    auto cr = Cairo::Context::create(surface);

    // Set the brush color and alpha
    cr->set_source_rgba(1.0, 1.0, 1.0, m_brushAlpha);

    // Draw the circle representing the brush
    cr->arc(x, y, brushRadius, 0, 2 * M_PI);
    cr->fill();

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();
}