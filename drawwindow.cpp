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

void DrawWindow::on_window_shown() {
    std::cout << "Window is shown!" << std::endl;
    // Load the first image buffer to drawing area
    if (!m_imageLabelingQueue.empty())
    {
        loadImageBuffer(m_imageLabelingQueue.front());
    }
}

bool DrawWindow::onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context>& cr) {
    if (m_currentPixbuf) {
        // Get the dimensions of the pixbuf
        int width = m_currentPixbuf->get_width();
        int height = m_currentPixbuf->get_height();

        // Set the size of the drawing area if needed
        //m_drawingArea->set_size_request(width, height);

        // Draw the loaded pixbuf
        Gdk::Cairo::set_source_pixbuf(cr, m_currentPixbuf, 0, 0);
        cr->paint();  // Render the image
    }
    return true;  // Return true to indicate the event has been handled
}

void DrawWindow::loadImageBuffer(const std::string &filename)
{
    try
    {
        m_currentPixbuf = Gdk::Pixbuf::create_from_file(filename);

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