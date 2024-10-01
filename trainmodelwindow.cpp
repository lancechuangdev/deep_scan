#include "trainmodelwindow.h"

TrainModelWindow::TrainModelWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &TrainModelWindow::on_window_shown));

    m_refGlade->get_widget("train_model_images_count_lbl", m_imagesCountLbl);
    m_refGlade->get_widget("train_model_masks_count_lbl", m_masksCountLbl);
    m_refGlade->get_widget("train_model_images_path_lbl", m_imagesPathLbl);
    m_refGlade->get_widget("train_model_masks_path_lbl", m_masksPathLbl);
    m_refGlade->get_widget("train_model_path_lbl", m_modelPathLbl);
    m_refGlade->get_widget("training_error_msg_lbl", m_errorMsgLbl);
    m_refGlade->get_widget("model_patch_size_sb", m_patchSizeSb);
    if (m_patchSizeSb)
    {
        m_patchSizeSb->signal_value_changed().connect([this]()
                                                      { m_patchSize = static_cast<int>(m_patchSizeSb->get_value()); });
    }
    m_refGlade->get_widget("start_training_btn", m_startTrainingBtn);
    if (m_startTrainingBtn)
    {
        m_startTrainingBtn->signal_clicked().connect(sigc::mem_fun(*this, &TrainModelWindow::onStartTrainingClicked));
    }
}

TrainModelWindow *TrainModelWindow::create(const std::string &gladeFilePath)
{
    // Load the Glade file
    auto refBuilder = Gtk::Builder::create_from_file(gladeFilePath);

    // Get the window object from the Glade file
    TrainModelWindow *window = nullptr;
    refBuilder->get_widget_derived("train_model_window", window);

    return window;
}

void TrainModelWindow::setModelPath(const std::string &imagesPath, const std::string &masksPath, const std::string &modelPath)
{
    m_imagesPath = imagesPath;
    m_masksPath = masksPath;

    m_selectedImages = FileUtils::getImageFiles(imagesPath);
    m_selectedMasks = FileUtils::getImageFiles(masksPath);
    // Get current time and format it as YYYYMMDD_HHMMSS
    char timestamp[20];
    std::time_t now = std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    std::string timestampStr(timestamp);
    std::string modelFolder = "model_" + timestampStr;
    bool isModelDirCreated = FileUtils::createSubdirectory(modelPath, modelFolder);
    if (!isModelDirCreated)
    {
        std::cerr << "Failed to create 'model' directory. Exiting function." << std::endl;
    }
    else
    {
        m_modelPath = Glib::build_filename(modelPath, modelFolder);
    }
}

void TrainModelWindow::on_window_shown()
{
    this->set_title(Glib::ustring("Train Model"));

    if (m_imagesCountLbl)
    {
        m_imagesCountLbl->set_text(Glib::ustring::compose("Load %1 Images in:", m_selectedImages.size()));
    }
    if (m_masksCountLbl)
    {
        m_masksCountLbl->set_text(Glib::ustring::compose("Load %1 Masks in:", m_selectedMasks.size()));
    }
    if (m_imagesPathLbl)
    {
        m_imagesPathLbl->set_text(Glib::ustring(m_imagesPath));
    }
    if (m_masksPathLbl)
    {
        m_masksPathLbl->set_text(Glib::ustring(m_masksPath));
    }
    if (m_modelPathLbl)
    {
        m_modelPathLbl->set_text(Glib::ustring(m_modelPath));
    }
    if (m_patchSizeSb)
    {
        m_patchSize = m_patchSizeSb->get_value();
    }
}

void TrainModelWindow::onStartTrainingClicked()
{
    // Reset the error message
    m_errorMsgLbl->set_text(Glib::ustring(""));

    if (!validateDataset())
    {
        if (m_errorMsgLbl)
        {
            m_errorMsgLbl->set_text(Glib::ustring("Error occurs when loading the selected dataset for training."));
        }
    }
    else
    {
        // TODO
    }
}

bool TrainModelWindow::validateDataset()
{
    return m_selectedImages.size() == m_selectedMasks.size() &&
    FileUtils::checkImagesDimensions(m_selectedImages, m_patchSize, m_patchSize) &&
    FileUtils::checkImagesDimensions(m_selectedMasks, m_patchSize, m_patchSize) &&
    FileUtils::checkImagesHaveMasks(m_selectedImages, m_selectedMasks);
}