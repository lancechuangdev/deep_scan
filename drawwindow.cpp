#include "drawwindow.h"

double calculateLabelingProgress(const std::string &folder_path)
{
    int total_images = 0;
    int masked_images = 0;

    // Iterate over files in the folder
    for (const auto &entry : std::filesystem::directory_iterator(folder_path))
    {
        // Check if it's a regular file
        if (entry.is_regular_file())
        {
            std::string file_name = entry.path().filename().string();

            // Check if it's an image file (excluding masks)
            if (file_name.find("_mask") == std::string::npos)
            {
                total_images++;
                // Check if corresponding mask file exists
                std::string mask_file = entry.path().stem().string() + "_mask" + entry.path().extension().string();
                if (std::filesystem::exists(folder_path + "/" + mask_file))
                {
                    masked_images++;
                }
            }
        }
    }

    // Calculate the progress as a percentage
    if (total_images == 0)
    {
        return 0.0; // To avoid division by zero
    }
    return static_cast<double>(masked_images) / total_images;
}

DrawWindow::DrawWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &DrawWindow::on_window_shown));

    m_refGlade->get_widget("label_pb", m_labelingPb);

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

    m_refGlade->get_widget("image_name_lbl", m_imageNameLbl);
    m_refGlade->get_widget("image_paging_lbl", m_imagePagingLbl);

    m_refGlade->get_widget("previous_image_btn", m_previousImageBtn);
    if (m_previousImageBtn)
    {
        m_previousImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &DrawWindow::onPreviousImageClicked));
    }

    m_refGlade->get_widget("next_image_btn", m_nextImageBtn);
    if (m_nextImageBtn)
    {
        m_nextImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &DrawWindow::onNextImageClicked));
    }

    m_refGlade->get_widget("mask_switch", m_maskSwitch);
    if (m_maskSwitch)
    {
        // Get the PropertyProxy for the active property of the switch
        Glib::PropertyProxy<bool> active_property = m_maskSwitch->property_active();

        // Connect to the signal_changed() of the PropertyProxy
        active_property.signal_changed().connect(sigc::mem_fun(*this, &DrawWindow::onMaskSwitchActiveChanged));
    }

    m_refGlade->get_widget("draw_select_rbtn", m_selectRadioBtn);
    if (m_selectRadioBtn)
    {
        m_selectRadioBtn->signal_toggled().connect(sigc::mem_fun(*this, &DrawWindow::onSelectToggled));
    }
    
    m_refGlade->get_widget("draw_brush_rbtn", m_brushRadioBtn);
    if (m_brushRadioBtn)
    {
        m_brushRadioBtn->signal_toggled().connect(sigc::mem_fun(*this, &DrawWindow::onBrushToggled));
    }

    m_refGlade->get_widget("reset_mask_btn", m_resetMaskBtn);
    if (m_resetMaskBtn)
    {
        m_resetMaskBtn->signal_clicked().connect(sigc::mem_fun(*this, &DrawWindow::onResetMaskClicked));
    }

    m_refGlade->get_widget("save_mask_btn", m_saveMaskBtn);
    if (m_saveMaskBtn)
    {
        m_saveMaskBtn->signal_clicked().connect(sigc::mem_fun(*this, &DrawWindow::onSaveMaskClicked));
    }
}

DrawWindow *DrawWindow::create(const std::string &gladeFilePath)
{
    // Load the Glade file
    auto refBuilder = Gtk::Builder::create_from_file(gladeFilePath);

    // Get the window object from the Glade file
    DrawWindow *window = nullptr;
    refBuilder->get_widget_derived("draw_window", window);

    return window;
}

void DrawWindow::setImageLabelingPath(const std::string &path)
{
    m_imageLabelingPath = path;
    m_imageLabelingQueue = FileUtils::getImageFiles(path);
    m_imageLabelingIndex = 0;
}

void DrawWindow::on_window_shown()
{
    this->set_title(Glib::ustring::compose("Label Images in %1", m_imageLabelingPath));

    // Update the labeling progress bar
    auto progress = calculateLabelingProgress(m_imageLabelingPath);
    if (m_labelingPb)
    {
        m_labelingPb->set_fraction(progress);
    }

    // Set image name label
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_imageLabelingQueue[m_imageLabelingIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_imageLabelingIndex + 1, m_imageLabelingQueue.size()));
    }

    // Update navigation buttons status
    if (m_previousImageBtn)
    {
        m_previousImageBtn->set_sensitive(m_imageLabelingIndex > 0);
    }

    if (m_nextImageBtn)
    {
        m_nextImageBtn->set_sensitive(m_imageLabelingIndex < m_imageLabelingQueue.size() - 1);
    }

    // Load the first image buffer to drawing area
    if (!m_imageLabelingQueue.empty())
    {
        loadDrawingAreaBuffer();
    }
}

bool DrawWindow::onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offsetX, m_offsetY);   // Apply panning offset
    cr->scale(m_zoomFactor, m_zoomFactor); // Apply zoom

    // Draw the image
    if (m_ImagePixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_ImagePixbuf, 0, 0);
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

void DrawWindow::InitializeMaskPixBuf(int width, int height)
{
    // Create a transparent mask pixbuf of the same size as the image
    m_maskPixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
    // m_maskPixbuf->fill(0xffffffbe); // For testing
    m_maskPixbuf->fill(0x00000000); // Initialize the mask to be fully transparent black
}

void DrawWindow::clearDrawingArea()
{
    if (m_ImagePixbuf)
    {
        int width = m_ImagePixbuf->get_width();
        int height = m_ImagePixbuf->get_height();

        m_ImagePixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
        m_ImagePixbuf->fill(0x00000000); // Initialize the mask to be fully transparent black

        InitializeMaskPixBuf(width, height);
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();
}

void DrawWindow::loadDrawingAreaBuffer(bool showMask)
{
    if (!m_drawingArea)
        return;

    // Clear the drawing area before loading
    clearDrawingArea();

    // Reset zoom and pan when a new image is loaded
    m_zoomFactor = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;

    loadImageBufferFromFile(m_imageLabelingQueue[m_imageLabelingIndex]);

    int width, height;
    m_drawingArea->get_size_request(width, height);

    if (m_ImagePixbuf)
    {
        // Get the dimensions of the pixbuf
        width = m_ImagePixbuf->get_width();
        height = m_ImagePixbuf->get_height();
        // Set the size of the drawing area if needed
        m_drawingArea->set_size_request(width, height);
    }

    // Load mask pix buf
    InitializeMaskPixBuf(width, height);

    if (showMask)
    {
        auto maskFile = FileUtils::constructMaskPath(m_imageLabelingQueue[m_imageLabelingIndex]);
        if (std::filesystem::exists(maskFile))
        {
            LoadMaskBufferFromFile(maskFile);
        }
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();
}

void DrawWindow::loadImageBufferFromFile(const std::string &filename)
{
    try
    {
        // Load image
        m_ImagePixbuf = Gdk::Pixbuf::create_from_file(filename);
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

void DrawWindow::LoadMaskBufferFromFile(const std::string &filename)
{
    try
    {
        // Load mask
        m_maskPixbuf = Gdk::Pixbuf::create_from_file(filename);

        // Ensure the pixbuf has an alpha channel
        if (!m_maskPixbuf->get_has_alpha())
        {
            // Add alpha channel if it's not present
            m_maskPixbuf = m_maskPixbuf->add_alpha(false, 0, 0, 0);
        }

        // Get pixbuf properties
        int width = m_maskPixbuf->get_width();
        int height = m_maskPixbuf->get_height();
        int rowstride = m_maskPixbuf->get_rowstride();
        int n_channels = m_maskPixbuf->get_n_channels();

        // Get pointer to the pixel data
        guchar *pixels = m_maskPixbuf->get_pixels();

        // Iterate through the pixels and modify the alpha channel
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                guchar *pixel = pixels + y * rowstride + x * n_channels;

                // Check if the pixel is black (RGB = 0,0,0)
                if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
                {
                    // Set alpha to 0 (fully transparent)
                    pixel[3] = 0;
                }
                else if (pixel[0] > 0 && pixel[1] > 0 && pixel[2] > 0)
                {
                    pixel[3] = m_brushAlpha * 255;
                }
            }
        }
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

void DrawWindow::onPreviousImageClicked()
{
    m_imageLabelingIndex = std::max(static_cast<size_t>(0), m_imageLabelingIndex - 1);
    m_previousImageBtn->set_sensitive(m_imageLabelingIndex > 0);
    m_nextImageBtn->set_sensitive(m_imageLabelingIndex < m_imageLabelingQueue.size() - 1);
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_imageLabelingQueue[m_imageLabelingIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_imageLabelingIndex + 1, m_imageLabelingQueue.size()));
    }
    if (m_maskSwitch)
    {
        loadDrawingAreaBuffer(m_maskSwitch->get_active());
    }
}

void DrawWindow::onNextImageClicked()
{
    m_imageLabelingIndex = std::min(m_imageLabelingQueue.size() - 1, m_imageLabelingIndex + 1);
    m_previousImageBtn->set_sensitive(m_imageLabelingIndex > 0);
    m_nextImageBtn->set_sensitive(m_imageLabelingIndex < m_imageLabelingQueue.size() - 1);
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_imageLabelingQueue[m_imageLabelingIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_imageLabelingIndex + 1, m_imageLabelingQueue.size()));
    }
    if (m_maskSwitch)
    {
        loadDrawingAreaBuffer(m_maskSwitch->get_active());
    }
}

void DrawWindow::onMaskSwitchActiveChanged()
{
    bool showMask = m_maskSwitch->get_active(); // Retrieve the current state
    loadDrawingAreaBuffer(showMask);
}

void DrawWindow::onSelectToggled()
{
    // Handled in 'onPatchToggled' since they are mutually exclusive
}

void DrawWindow::onBrushToggled()
{
    if (m_brushRadioBtn->get_active())
    {
        m_isDrawingMode = true;
        m_showBrushCursor = true;
    }
    else
    {
        m_isDrawingMode = false;
        m_showBrushCursor = false;
    }
}

void DrawWindow::onResetMaskClicked()
{
    if (m_ImagePixbuf)
    {
        int width = m_ImagePixbuf->get_width();
        int height = m_ImagePixbuf->get_height();
        InitializeMaskPixBuf(width, height);

        // Trigger a redraw of the drawing area
        m_drawingArea->queue_draw();
    }
}

void DrawWindow::onSaveMaskClicked()
{
    auto filename = FileUtils::constructMaskPath(m_imageLabelingQueue[m_imageLabelingIndex]);
    saveMaskAsBinary(filename);

    auto progress = calculateLabelingProgress(m_imageLabelingPath);
    if (m_labelingPb)
    {
        m_labelingPb->set_fraction(progress);
    }
}

void DrawWindow::saveMaskAsBinary(const std::string &filename)
{
    // Get the pixel data from the mask pixbuf
    guchar *pixels = m_maskPixbuf->get_pixels();
    int width = m_maskPixbuf->get_width();
    int height = m_maskPixbuf->get_height();
    int rowstride = m_maskPixbuf->get_rowstride();
    int n_channels = m_maskPixbuf->get_n_channels();

    // Create a new grayscale Cairo surface to store the binary mask
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_A8, width, height);
    auto cr = Cairo::Context::create(surface);

    // Access the surface's pixel data to manipulate the binary mask
    unsigned char *surface_data = surface->get_data();

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Access the pixel in the original pixbuf (mask pixbuf)
            guchar *pixel = pixels + y * rowstride + x * n_channels;
            guchar alpha = pixel[3]; // Alpha channel

            // If alpha is above a threshold (drawn), set the corresponding pixel to 1, otherwise 0
            unsigned char mask_value = (alpha > 0) ? 1 : 0;

            // Write the binary mask value to the surface
            surface_data[y * width + x] = mask_value * 255; // For visualization, multiply by 255
        }
    }

    // Mark the surface as modified to ensure changes are reflected
    surface->mark_dirty();

    // Save the surface as a grayscale image (e.g., PNG, BMP, etc.)
    surface->write_to_png(filename);
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

            UpdateMaskAlpha(m_brushAlpha * 255);
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

            drawOnMask();
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
        m_brushX = (motion_event->x - m_offsetX) / m_zoomFactor;
        m_brushY = (motion_event->y - m_offsetY) / m_zoomFactor;

        if (m_isDrawing)
        {
            drawOnMask();
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

void DrawWindow::drawOnMask()
{
    // Get pixbuf pixel data
    guchar *pixels = m_maskPixbuf->get_pixels();
    int rowstride = m_maskPixbuf->get_rowstride();
    int n_channels = m_maskPixbuf->get_n_channels();

    // Loop over the circular area where the brush/eraser is applied
    for (int i = -m_brushRadius; i <= m_brushRadius; ++i)
    {
        for (int j = -m_brushRadius; j <= m_brushRadius; ++j)
        {
            int brushX = m_brushX + i;
            int brushY = m_brushY + j;

            // Ensure we're within the bounds of the image
            if (brushX >= 0 && brushX < m_maskPixbuf->get_width() &&
                brushY >= 0 && brushY < m_maskPixbuf->get_height())
            {

                // Calculate the distance from the center of the brush
                double distance = std::sqrt(i * i + j * j);
                if (distance <= m_brushRadius)
                {
                    // Get a pointer to the current pixel
                    guchar *pixel = pixels + brushY * rowstride + brushX * n_channels;
                    if (m_ctrlPressed)
                    {
                        // Set color to black and alpha to 0 (transparent) to erase
                        pixel[0] = 0;
                        pixel[1] = 0;
                        pixel[2] = 0;
                        pixel[3] = 0;
                    }
                    else
                    {
                        // Set alpha and color for brush mode
                        pixel[0] = 255;                       // Red
                        pixel[1] = 255;                       // Green
                        pixel[2] = 255;                       // Blue
                        pixel[3] = (int)(m_brushAlpha * 255); // Set alpha
                    }
                }
            }
        }
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();
}

void DrawWindow::UpdateMaskAlpha(gint32 alpha)
{
    if (!m_maskPixbuf)
        return;

    // Get pixbuf properties
    int width = m_maskPixbuf->get_width();
    int height = m_maskPixbuf->get_height();
    int rowstride = m_maskPixbuf->get_rowstride();
    int n_channels = m_maskPixbuf->get_n_channels();

    // Get pointer to the pixel data
    guchar *pixels = m_maskPixbuf->get_pixels();

    // Iterate through the pixels and modify the alpha channel
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            guchar *pixel = pixels + y * rowstride + x * n_channels;

            if (pixel[3] > 0)
            {
                pixel[3] = alpha;
            }
        }
    }
}