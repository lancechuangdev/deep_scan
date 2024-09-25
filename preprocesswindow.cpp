#include "preprocesswindow.h"

PreprocessWindow::PreprocessWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &PreprocessWindow::on_window_shown));

    m_refGlade->get_widget("preprocess_select_area_tbtn", m_selectAreaToggleBtn);
    if (m_selectAreaToggleBtn)
    {
        m_selectAreaToggleBtn->signal_toggled().connect(sigc::mem_fun(*this, &PreprocessWindow::onSelectedAreaToggled));
    }

    m_refGlade->get_widget("patch_settings_box", m_patchSettingsBox);
    m_refGlade->get_widget("preprocess_image_name_lbl", m_imageNameLbl);
    m_refGlade->get_widget("preprocess_image_paging_lbl", m_imagePagingLbl);

    m_refGlade->get_widget("preprocess_previous_image_btn", m_previousImageBtn);
    if (m_previousImageBtn)
    {
        m_previousImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &PreprocessWindow::onPreviousImageClicked));
    }

    m_refGlade->get_widget("preprocess_next_image_btn", m_nextImageBtn);
    if (m_nextImageBtn)
    {
        m_nextImageBtn->signal_clicked().connect(sigc::mem_fun(*this, &PreprocessWindow::onNextImageClicked));
    }

    m_refGlade->get_widget("preprocess_drawing_area", m_drawingArea);
    if (m_drawingArea)
    {
        m_drawingArea->signal_draw().connect(sigc::mem_fun(*this, &PreprocessWindow::onDrawingAreaDraw));
    }

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
        //loadDrawingAreaBuffer();
    }
}

void PreprocessWindow::onPreviousImageClicked()
{

}

void PreprocessWindow::onNextImageClicked()
{

}

void PreprocessWindow::onSelectedAreaToggled()
{
    if (m_selectAreaToggleBtn->get_active())
    {
        std::cout << "active";
        // Show popup when the button is toggled on
        m_patchSettingsBox->set_visible(true);
    }
    else
    {
        // Hide popup when the button is toggled off
        m_patchSettingsBox->set_visible(false);
    }
}

bool PreprocessWindow::onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr)
{

}