#include "preprocesswindow.h"

PreprocessWindow::PreprocessWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &PreprocessWindow::on_window_shown));

    m_refGlade->get_widget("patch_select_rbtn", m_selectRadioBtn);
    if (m_selectRadioBtn)
    {
        m_selectRadioBtn->signal_toggled().connect(sigc::mem_fun(*this, &PreprocessWindow::onSelectToggled));
    }

    m_refGlade->get_widget("patch_roi_rbtn", m_selectROIRadioBtn);
    if (m_selectROIRadioBtn)
    {
        m_selectROIRadioBtn->signal_toggled().connect(sigc::mem_fun(*this, &PreprocessWindow::onPatchToggled));
    }

    m_refGlade->get_widget("patch_settings_box", m_patchSettingsBox);
    m_refGlade->get_widget("patch_image_name_lbl", m_imageNameLbl);
    m_refGlade->get_widget("patch_image_paging_lbl", m_imagePagingLbl);

    m_refGlade->get_widget("patch_previous_image_btn", m_previousImageBtn);
    if (m_previousImageBtn)
    {
        m_previousImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &PreprocessWindow::onPreviousImageClicked));
    }

    m_refGlade->get_widget("patch_next_image_btn", m_nextImageBtn);
    if (m_nextImageBtn)
    {
        m_nextImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &PreprocessWindow::onNextImageClicked));
    }

    m_refGlade->get_widget("patch_drawing_area", m_drawingArea);
    if (m_drawingArea)
    {
        m_drawingArea->signal_draw().connect(sigc::mem_fun(*this, &PreprocessWindow::onDrawingAreaDraw));

        // Connect mouse scroll event
        m_drawingArea->add_events(Gdk::SCROLL_MASK);
        m_drawingArea->signal_scroll_event().connect(sigc::mem_fun(*this, &PreprocessWindow::onScrollEvent));

        // Connect mouse press and motion events
        m_drawingArea->add_events(Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::POINTER_MOTION_MASK);
        m_drawingArea->signal_button_press_event().connect(sigc::mem_fun(*this, &PreprocessWindow::onButtonPressEvent));
        m_drawingArea->signal_button_release_event().connect(sigc::mem_fun(*this, &PreprocessWindow::onButtonReleaseEvent));
        m_drawingArea->signal_motion_notify_event().connect(sigc::mem_fun(*this, &PreprocessWindow::onMotionNotifyEvent));

        // Connect key press events
        m_drawingArea->add_events(Gdk::KEY_PRESS_MASK);
    }

    m_refGlade->get_widget("patch_size_sb", m_patchSizeSb);
    if (m_patchSizeSb)
    {
        m_patchSizeSb->signal_value_changed().connect([this]()
                                                      { m_patchSize = static_cast<int>(m_patchSizeSb->get_value()); });
    }
    m_refGlade->get_widget("patch_width_sb", m_patchWidthSb);
    if (m_patchWidthSb)
    {
        m_patchWidthSb->signal_value_changed().connect([this]()
                                                       { m_patchWidth = static_cast<int>(m_patchWidthSb->get_value()); });
    }
    m_refGlade->get_widget("patch_height_sb", m_patchHeightSb);
    if (m_patchHeightSb)
    {
        m_patchHeightSb->signal_value_changed().connect([this]()
                                                        { m_patchHeight = static_cast<int>(m_patchHeightSb->get_value()); });
    }
    m_refGlade->get_widget("thumbnails_listbox", m_thumbnailsListbox);
}

PreprocessWindow *PreprocessWindow::create(const std::string &gladeFilePath)
{
    // Load the Glade file
    auto refBuilder = Gtk::Builder::create_from_file(gladeFilePath);

    // Get the window object from the Glade file
    PreprocessWindow *window = nullptr;
    refBuilder->get_widget_derived("preprocess_window", window);

    return window;
}

void PreprocessWindow::setImagePreprocessingPath(const std::string &path)
{
    m_preprocessImagePath = path;
    m_preprocessImageQueue = FileUtils::getImageFiles(path);
    m_preprocessImageIndex = 0;
}

void PreprocessWindow::on_window_shown()
{
    this->set_title(Glib::ustring::compose("Preprocess Images in %1", m_preprocessImagePath));

    if (m_preprocessImageQueue.empty())
    {
        return;
    }

    // Set image name label
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_preprocessImageQueue[m_preprocessImageIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_preprocessImageIndex + 1, m_preprocessImageQueue.size()));
    }

    // Update navigation buttons status
    if (m_previousImageBtn)
    {
        m_previousImageBtn->set_sensitive(m_preprocessImageIndex > 0);
    }

    if (m_nextImageBtn)
    {
        m_nextImageBtn->set_sensitive(m_preprocessImageIndex < m_preprocessImageQueue.size() - 1);
    }

    // Load the first image buffer to drawing area
    if (!m_preprocessImageQueue.empty())
    {
        loadDrawingAreaBuffer();
        loadPatchThumbnails();
    }
}

void PreprocessWindow::loadPatchThumbnails()
{
    // Clear the thumbsnails before loading
    for (auto *child : m_thumbnailsListbox->get_children())
    {
        m_thumbnailsListbox->remove(*child);
    }

    std::filesystem::path path(m_preprocessImageQueue[m_preprocessImageIndex]);
    std::string parent = Glib::path_get_dirname(path);
    if (FileUtils::directoryExists(parent, "images") && FileUtils::directoryExists(parent, "masks"))
    {
        // Get the base name (filename with extension)
        std::string baseName = Glib::path_get_basename(path); // Get filename with extension

        // Remove the extension from base name
        baseName = baseName.substr(0, baseName.length() - FileUtils::get_extension(baseName).length());

        // Find all images in 'images' dir that matches this pattern 'baseName_<any characters>.extension'
        std::string imagesDir = Glib::build_filename(parent, "images");
        std::regex imagesPattern(baseName + "_.*\\.(png|jpe?g|bmp)"); // Matches 'baseName_<any characters>.extension'
        auto images = FileUtils::findMatchingImages(imagesDir, baseName, imagesPattern);

        // Find all images in 'masks' dir that matches this pattern 'baseName_<any characters>_mask.extension'
        std::string masksDir = Glib::build_filename(parent, "masks");
        std::regex masksPattern(baseName + "_.*_mask\\..*"); // Convert the string to a regex
        auto masks = FileUtils::findMatchingImages(masksDir, baseName, masksPattern);

        // Sort images and masks by name then add a pair of image/mask
        if (images.size() != masks.size())
        {
            return;
        }

        std::vector<std::string> sortedImages = images;
        std::vector<std::string> sortedMasks = masks;

        std::sort(sortedImages.begin(), sortedImages.end());
        std::sort(sortedMasks.begin(), sortedMasks.end());

        // Iterate over both sorted images and masks
        for (size_t i = 0; i < sortedImages.size() && i < sortedMasks.size(); ++i)
        {
            const std::string &image = sortedImages[i];
            const std::string &mask = sortedMasks[i];

            // Assuming the masks have the correct corresponding order after sorting
            addThumbnailsToList(image, mask);
        }
    }
}

void PreprocessWindow::loadDrawingAreaBuffer()
{
    if (!m_drawingArea)
        return;

    // Clear the drawing area before loading
    m_imagePixbuf.reset();
    m_maskPixbuf.reset();

    // Reset zoom and pan when a new image is loaded
    m_zoomFactor = 1.0;
    m_offsetX = 0.0;
    m_offsetY = 0.0;

    loadImageBufferFromFile(m_preprocessImageQueue[m_preprocessImageIndex]);

    int width, height;
    m_drawingArea->get_size_request(width, height);

    if (m_imagePixbuf)
    {
        // Get the dimensions of the pixbuf
        width = m_imagePixbuf->get_width();
        height = m_imagePixbuf->get_height();
        // Set the size of the drawing area if needed
        m_drawingArea->set_size_request(width, height);
    }

    // Load mask pix buf
    auto maskFile = FileUtils::constructMaskPath(m_preprocessImageQueue[m_preprocessImageIndex]);
    if (std::filesystem::exists(maskFile))
    {
        LoadMaskBufferFromFile(maskFile);
    }

    // Trigger a redraw of the drawing area
    m_drawingArea->queue_draw();
}

void PreprocessWindow::loadImageBufferFromFile(const std::string &filename)
{
    try
    {
        // Load image
        m_imagePixbuf = Gdk::Pixbuf::create_from_file(filename);
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

void PreprocessWindow::LoadMaskBufferFromFile(const std::string &filename)
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
                    pixel[0] = AmberRed;
                    pixel[1] = AmberGreen;
                    pixel[2] = AmberBlue;
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

void PreprocessWindow::onPreviousImageClicked()
{
    m_preprocessImageIndex = std::max(static_cast<size_t>(0), m_preprocessImageIndex - 1);
    m_previousImageBtn->set_sensitive(m_preprocessImageIndex > 0);
    m_nextImageBtn->set_sensitive(m_preprocessImageIndex < m_preprocessImageQueue.size() - 1);
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_preprocessImageQueue[m_preprocessImageIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_preprocessImageIndex + 1, m_preprocessImageQueue.size()));
    }

    if (!m_preprocessImageQueue.empty())
    {
        loadDrawingAreaBuffer();
        loadPatchThumbnails();
    }
}

void PreprocessWindow::onNextImageClicked()
{
    m_preprocessImageIndex = std::min(m_preprocessImageQueue.size() - 1, m_preprocessImageIndex + 1);
    m_previousImageBtn->set_sensitive(m_preprocessImageIndex > 0);
    m_nextImageBtn->set_sensitive(m_preprocessImageIndex < m_preprocessImageQueue.size() - 1);
    if (m_imageNameLbl)
    {
        std::filesystem::path path(m_preprocessImageQueue[m_preprocessImageIndex]);
        std::string file_name = path.filename().string();
        m_imageNameLbl->set_text(Glib::ustring(file_name));
    }
    if (m_imagePagingLbl)
    {
        m_imagePagingLbl->set_text(Glib::ustring::compose("%1 of %2", m_preprocessImageIndex + 1, m_preprocessImageQueue.size()));
    }

    if (!m_preprocessImageQueue.empty())
    {
        loadDrawingAreaBuffer();
        loadPatchThumbnails();
    }
}

void PreprocessWindow::onSelectToggled()
{
    // Handled in 'onPatchToggled' since they are mutually exclusive
}

void PreprocessWindow::onPatchToggled()
{
    if (m_selectROIRadioBtn->get_active())
    {
        m_isPatchingMode = true;
        m_showPatchCursor = true;
    }
    else
    {
        m_isPatchingMode = false;
        m_showPatchCursor = false;
    }
}

bool PreprocessWindow::onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr)
{
    // Apply zoom and pan transformations
    cr->translate(m_offsetX, m_offsetY);   // Apply panning offset
    cr->scale(m_zoomFactor, m_zoomFactor); // Apply zoom

    // Draw the image
    if (m_imagePixbuf)
    {
        Gdk::Cairo::set_source_pixbuf(cr, m_imagePixbuf, 0, 0);
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

void PreprocessWindow::drawBrushCursor(const Cairo::RefPtr<Cairo::Context> &cr)
{
    if (m_showPatchCursor)
    {
        // Calculate the top-left corner of the big rectangle
        int width = m_patchSize * m_patchWidth;
        int height = m_patchSize * m_patchHeight;
        double rect_x = m_brushX - width;
        double rect_y = m_brushY - height;

        // Set line width and color for the solid square outline
        cr->set_source_rgb(1.0, 0.0, 0.0); // Red color for the outline

        // Set dash pattern for dotted lines
        std::vector<double> dashes = {5.0, 5.0}; // 5 pixels on, 5 pixels off
        cr->set_dash(dashes, 0);                 // Apply dash pattern

        // Draw the smaller squares with dotted lines within the big rectangle
        for (int i = 0; i < m_patchWidth; ++i)
        {
            for (int j = 0; j < m_patchHeight; ++j)
            {
                // Calculate top-left corner of each smaller square
                double small_rect_x = rect_x + i * m_patchSize;
                double small_rect_y = rect_y + j * m_patchSize;

                // Draw the top side
                cr->move_to(small_rect_x, small_rect_y);
                cr->line_to(small_rect_x + m_patchSize, small_rect_y);

                // Draw the left side
                cr->move_to(small_rect_x, small_rect_y);
                cr->line_to(small_rect_x, small_rect_y + m_patchSize);

                // For squares at last column, draw the right edges
                if (i == m_patchWidth - 1)
                {
                    cr->move_to(small_rect_x + m_patchSize, small_rect_y);
                    cr->line_to(small_rect_x + m_patchSize, small_rect_y + m_patchSize);
                }
                // For squares at last row, draw the bottom edges
                if (j == m_patchHeight - 1)
                {
                    cr->move_to(small_rect_x, small_rect_y + m_patchSize);
                    cr->line_to(small_rect_x + m_patchSize, small_rect_y + m_patchSize);
                }

                cr->stroke(); // Stroke to render the lines
            }
        }

        // Reset the dash pattern to solid line for future drawings
        cr->set_dash(std::vector<double>(), 0);
    }
}

// Handle mouse scroll
bool PreprocessWindow::onScrollEvent(GdkEventScroll *scroll_event)
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
    else if (!m_isPatchingMode)
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

void PreprocessWindow::UpdateMaskAlpha(gint32 alpha)
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

void PreprocessWindow::drawOnSelectedROI()
{
}

// Handle mouse press for starting panning
bool PreprocessWindow::onButtonPressEvent(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        if (m_isPatchingMode)
        {
            m_showPatchCursor = false;

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
bool PreprocessWindow::onButtonReleaseEvent(GdkEventButton *button_event)
{
    if (button_event->button == 1)
    {
        if (m_isPatchingMode && m_imagePixbuf && m_maskPixbuf)
        {
            m_showPatchCursor = true;

            // Save patch (selected ROI) to files
            std::filesystem::path path(m_preprocessImageQueue[m_preprocessImageIndex]);
            std::string dir = Glib::path_get_dirname(path);
            std::string patchName = "";
            std::string patchPath = "";
            std::string maskPath = "";
            bool isPatchDirCreated = false;
            bool isImageSaved = false;

            // Ensure the 'images' subdirectory exists, but do not override any existing images
            isPatchDirCreated = FileUtils::createSubdirectory(dir, "images");
            if (!isPatchDirCreated)
            {
                std::cerr << "Failed to create 'images' directory. Exiting function." << std::endl;
            }

            // Ensure the 'masks' subdirectory exists, but do not override any existing images
            isPatchDirCreated = FileUtils::createSubdirectory(dir, "masks");
            if (!isPatchDirCreated)
            {
                std::cerr << "Failed to create 'masks' directory. Exiting function." << std::endl;
            }

            if (isPatchDirCreated)
            {
                for (int i = m_patchWidth; i > 0; i--)
                {
                    for (int j = m_patchHeight; j > 0; j--)
                    {
                        patchName = FileUtils::constructPatchName(path, ".png");
                        patchPath = Glib::build_filename(dir, "images", patchName);
                        isImageSaved = saveImagePatch(patchPath, m_brushY - m_patchSize * j, m_brushX - m_patchSize * i);

                        if (isImageSaved)
                        {
                            std::string maskName = FileUtils::constructMaskName(patchName);
                            maskPath = Glib::build_filename(dir, "masks", maskName);
                            saveMaskAsBinary(maskPath, m_brushY - m_patchSize * j, m_brushX - m_patchSize * i);
                        }

                        if (!patchPath.empty() && !maskPath.empty())
                        {
                            addThumbnailsToList(patchPath, maskPath);
                        }
                    }
                }
            }

            m_drawingArea->queue_draw();
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
bool PreprocessWindow::onMotionNotifyEvent(GdkEventMotion *motion_event)
{
    if (m_isPatchingMode)
    {
        m_brushX = (motion_event->x - m_offsetX) / m_zoomFactor;
        m_brushY = (motion_event->y - m_offsetY) / m_zoomFactor;
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

bool PreprocessWindow::on_key_press_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrlPressed = true;
    }
    return Gtk::Window::on_key_press_event(key_event);
}

bool PreprocessWindow::on_key_release_event(GdkEventKey *key_event)
{
    if (key_event->keyval == GDK_KEY_Control_L || key_event->keyval == GDK_KEY_Control_R)
    {
        m_ctrlPressed = false;
    }
    return Gtk::Window::on_key_release_event(key_event);
}

bool PreprocessWindow::loadMetadata(const std::string &metadataFilename, int &patchSize, double &top, double &left)
{
    std::ifstream metadataFile(metadataFilename);
    if (!metadataFile.is_open())
    {
        std::cerr << "Error: Could not open metadata file for reading." << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(metadataFile, line))
    {
        if (line.find("Patch Size:") != std::string::npos)
        {
            patchSize = std::stod(line.substr(line.find(':') + 1));
        }
        else if (line.find("Top:") != std::string::npos)
        {
            top = std::stod(line.substr(line.find(':') + 1));
        }
        else if (line.find("Left:") != std::string::npos)
        {
            left = std::stod(line.substr(line.find(':') + 1));
        }
    }

    metadataFile.close(); // Don't forget to close the file
    return true;
}

bool PreprocessWindow::saveMetadata(const std::string &metadataFilename, int patchSize, double top, double left)
{
    std::ofstream metadataFile(metadataFilename);
    if (!metadataFile.is_open())
    {
        std::cerr << "Error: Couldn't open metadata file for writing." << std::endl;
        return false;
    }

    metadataFile << "Patch Size: " << patchSize << std::endl;
    metadataFile << "Top: " << top << std::endl;
    metadataFile << "Left: " << left << std::endl;

    metadataFile.close();
    return true;
}

bool PreprocessWindow::saveImagePatch(const std::string &filename, double top, double left)
{
    // Get the pixel data from the mask pixbuf
    guchar *pixels = m_imagePixbuf->get_pixels();
    int rowstride = m_imagePixbuf->get_rowstride();
    int n_channels = m_imagePixbuf->get_n_channels();

    // Create a new grayscale Cairo surface to store the binary mask
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_A8, m_patchSize, m_patchSize);
    auto cr = Cairo::Context::create(surface);

    // Access the surface's pixel data to manipulate the binary mask
    unsigned char *surface_data = surface->get_data();

    for (int y = 0; y < m_patchSize; ++y)
    {
        for (int x = 0; x < m_patchSize; ++x)
        {
            // Transform to drawing area coordinates
            int y2 = y + top;
            int x2 = x + left;

            // early quite if the selected ROI is out of boundary
            if (x2 < 0 || x2 > m_imagePixbuf->get_width() || y2 < 0 || y2 > m_imagePixbuf->get_height())
            {
                return false;
            }

            // Access the pixel in the original pixbuf (mono8 format)
            guchar *pixel = pixels + y2 * rowstride + x2 * n_channels;
            guchar intensity = pixel[0]; // Grayscale intensity (mono8 format)

            // Write the grayscale intensity to the Cairo surface
            // surface_data[y * m_patchSize + x] = intensity;
            surface_data[y * m_patchSize + x] = std::min(255, std::max(0, static_cast<int>(intensity)));
        }
    }

    // Mark the surface as modified to ensure changes are reflected
    surface->mark_dirty();

    // Save the surface as a grayscale image (e.g., PNG, BMP, etc.)
    surface->write_to_png(filename);

    // Save image metadata
    std::string metadatafileName = FileUtils::replaceExtension(filename, "txt");
    return saveMetadata(metadatafileName, m_patchSize, top, left);
}

void PreprocessWindow::saveMaskAsBinary(const std::string &filename, double top, double left)
{
    if (!m_maskPixbuf)
    {
        return;
    }

    // Get the pixel data from the mask pixbuf
    guchar *pixels = m_maskPixbuf->get_pixels();
    int rowstride = m_maskPixbuf->get_rowstride();
    int n_channels = m_maskPixbuf->get_n_channels();

    // Create a new grayscale Cairo surface to store the binary mask
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_A8, m_patchSize, m_patchSize);
    auto cr = Cairo::Context::create(surface);

    // Access the surface's pixel data to manipulate the binary mask
    unsigned char *surface_data = surface->get_data();

    for (int y = 0; y < m_patchSize; ++y)
    {
        for (int x = 0; x < m_patchSize; ++x)
        {
            // Transform to drawing area coordinates
            int y2 = y + top;
            int x2 = x + left;

            // early quite if the selected ROI is out of boundary
            if (x2 < 0 || x2 > m_maskPixbuf->get_width() || y2 < 0 || y2 > m_maskPixbuf->get_height())
            {
                return;
            }

            // Access the pixel in the original pixbuf (mask pixbuf)
            guchar *pixel = pixels + y2 * rowstride + x2 * n_channels;
            guchar alpha = pixel[3]; // Alpha channel

            // If alpha is above a threshold (drawn), set the corresponding pixel to 1, otherwise 0
            unsigned char mask_value = (alpha > 0) ? 1 : 0;

            // Write the binary mask value to the surface
            surface_data[y * m_patchSize + x] = mask_value * 255; // For visualization, multiply by 255
        }
    }

    // Mark the surface as modified to ensure changes are reflected
    surface->mark_dirty();

    // Save the surface as a grayscale image (e.g., PNG, BMP, etc.)
    surface->write_to_png(filename);
}

void PreprocessWindow::addThumbnailsToList(const std::string &image, const std::string &mask)
{
    // Create a new Gtk::Box to hold the original image and mask side by side
    auto row_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL);

    // Load and scale down the original image to create a thumbnail (128x128)
    auto original_pixbuf = Gdk::Pixbuf::create_from_file(image);
    auto original_thumbnail = original_pixbuf->scale_simple(56, 56, Gdk::INTERP_BILINEAR);
    auto original_image = Gtk::make_managed<Gtk::Image>(original_thumbnail);

    // Load and scale down the mask image to create a thumbnail (128x128)
    auto mask_pixbuf = Gdk::Pixbuf::create_from_file(mask);
    auto mask_thumbnail = mask_pixbuf->scale_simple(56, 56, Gdk::INTERP_BILINEAR);
    auto mask_image = Gtk::make_managed<Gtk::Image>(mask_thumbnail);

    // Add the original image to the row box
    row_box->pack_start(*original_image, Gtk::PACK_SHRINK);

    // Add spacing between the images (e.g., 10 pixels)
    row_box->set_spacing(5);

    // Add the mask image to the row box
    row_box->pack_start(*mask_image, Gtk::PACK_SHRINK);

    // Add a show button to the row
    auto view_button = Gtk::make_managed<Gtk::Button>("view");
    view_button->set_margin_start(5);
    view_button->set_margin_end(5);
    view_button->set_margin_top(5);
    view_button->set_margin_bottom(5);
    view_button->signal_clicked().connect([this, row_box, image]()
                                            { onViewPatch(row_box, image); });
    row_box->pack_start(*view_button, Gtk::PACK_SHRINK);

    // Add a delete button to the row
    auto delete_button = Gtk::make_managed<Gtk::Button>("Delete");
    delete_button->set_margin_start(5);
    delete_button->set_margin_end(5);
    delete_button->set_margin_top(5);
    delete_button->set_margin_bottom(5);
    delete_button->signal_clicked().connect([this, row_box, image, mask]()
                                            { onDeleteRow(row_box, image, mask); });
    row_box->pack_start(*delete_button, Gtk::PACK_SHRINK);

    // Create a Gtk::ListBoxRow to wrap the box
    auto listbox_row = Gtk::make_managed<Gtk::ListBoxRow>();
    listbox_row->add(*row_box);

    // Add the Gtk::ListBoxRow to the list box
    m_thumbnailsListbox->append(*listbox_row);

    // Show all the newly added widgets
    listbox_row->show_all();
}

void PreprocessWindow::onDeleteRow(Gtk::Box *row_box, const std::string &image, const std::string &mask)
{
    // Remove the row from the list
    auto parent_row = static_cast<Gtk::ListBoxRow *>(row_box->get_parent());
    m_thumbnailsListbox->remove(*parent_row);

    // Delete the corresponding files    
    if (std::remove(image.c_str()) != 0)
    {
        std::cerr << "Error deleting original image file: " << image << std::endl;
    }

    if (std::remove(mask.c_str()) != 0)
    {
        std::cerr << "Error deleting mask file: " << mask << std::endl;
    }

    auto metadata = FileUtils::replaceExtension(image, "txt");
    if (std::remove(metadata.c_str()) != 0)
    {
        std::cerr << "Error deleting original image file: " << image << std::endl;
    }
}

void PreprocessWindow::onViewPatch(Gtk::Box *row_box, const std::string &image)
{
    int patchSize;
    double top, left;
    auto metadata = FileUtils::replaceExtension(image, "txt");
    if (loadMetadata(metadata, patchSize, top, left)) {
        m_patchSize = patchSize;
        m_brushY = top + patchSize;
        m_brushX = left + patchSize;

        m_patchSizeSb->set_value(patchSize);
        m_patchWidthSb->set_value(1);
        m_patchHeightSb->set_value(1);
        
        // Trigger a redraw of the drawing area
        m_drawingArea->queue_draw();
    } else {
        std::cerr << "Failed to load metadata." << std::endl;
    }
}