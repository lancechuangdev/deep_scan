#include "trainmodelwindow.h"

TrainModelWindow::TrainModelWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade)
    : Gtk::Window(cobject), m_refGlade(refGlade)
{
    signal_show().connect(sigc::mem_fun(*this, &TrainModelWindow::on_window_shown));

    m_refGlade->get_widget("train_model_images_count_lbl", m_imagesCountLbl);
    m_refGlade->get_widget("train_model_masks_count_lbl", m_masksCountLbl);
    m_refGlade->get_widget("train_model_images_path_lbl", m_imagesPathLbl);
    m_refGlade->get_widget("train_model_masks_path_lbl", m_masksPathLbl);
    m_refGlade->get_widget("training_error_msg_lbl", m_errorMsgLbl);

    m_refGlade->get_widget("model_patch_size_sb", m_patchSizeSb);
    if (m_patchSizeSb)
    {
        m_patchSizeSb->signal_value_changed().connect([this]() { m_patchSize = static_cast<int>(m_patchSizeSb->get_value()); });
    }

    m_refGlade->get_widget("model_batch_size_sb", m_batchSizeSb);
    if (m_batchSizeSb)
    {
        m_batchSizeSb->signal_value_changed().connect([this]() { m_batchSize = static_cast<int>(m_batchSizeSb->get_value()); });
    }

    m_refGlade->get_widget("epochs_sb", m_epochsSb);
    if (m_epochsSb)
    {
        m_epochsSb->signal_value_changed().connect([this]() { m_epochs = static_cast<int>(m_epochsSb->get_value()); });
    }

    m_refGlade->get_widget("pred_fidelity_sb",m_predFidelitySb);
    if (m_predFidelitySb)
    {
        m_predFidelitySb->signal_value_changed().connect([this]() { m_predFidelity = m_predFidelitySb->get_value(); });
    }

    m_refGlade->get_widget("py_env_entry", m_pyEnvEntry);
    if (m_pyEnvEntry)
    {
        m_pyEnvEntry->signal_changed().connect([this]() { m_pyEnv = m_pyEnvEntry->get_text(); });
    }

    m_refGlade->get_widget("start_training_btn", m_startTrainingBtn);
    if (m_startTrainingBtn)
    {
        m_startTrainingBtn->signal_clicked().connect(sigc::mem_fun(*this, &TrainModelWindow::onStartTrainingClicked));
    }

    m_refGlade->get_widget("view_model_lbtn", m_viewModelBtn);
    if (m_viewModelBtn)
    {
        m_viewModelBtn->signal_clicked().connect(sigc::mem_fun(*this, &TrainModelWindow::onViewModelClicked));
    }

    m_refGlade->get_widget("test_model_btn", m_testModelBtn);
    if (m_testModelBtn)
    {
        m_testModelBtn->signal_clicked().connect(sigc::mem_fun(*this, &TrainModelWindow::onTestModelClicked));
    }

    m_refGlade->get_widget("view_test_result_lbtn", m_viewTestResultBtn);
    if (m_viewTestResultBtn)
    {
        m_viewTestResultBtn->signal_clicked().connect(sigc::mem_fun(*this, &TrainModelWindow::onViewTestResultClicked));
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

void TrainModelWindow::setModelPath(const std::string &imagesPath, const std::string &masksPath)
{
    m_imagesPath = imagesPath;
    m_masksPath = masksPath;
    m_selectedImages = FileUtils::getImageFiles(imagesPath);
    m_selectedMasks = FileUtils::getImageFiles(masksPath);
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
    if (m_patchSizeSb)
    {
        m_patchSize = m_patchSizeSb->get_value();
    }
    if (m_batchSizeSb)
    {
        m_batchSize = m_batchSizeSb->get_value();
    }
    if (m_epochsSb)
    {
        m_epochs = m_epochsSb->get_value();
    }
    if (m_pyEnvEntry)
    {
        m_pyEnv = m_pyEnvEntry->get_text();
    }
}

bool TrainModelWindow::validateInputDataset()
{
    // return m_selectedImages.size() == m_selectedMasks.size() &&
    // FileUtils::checkImagesDimensions(m_selectedImages, m_patchSize, m_patchSize) &&
    // FileUtils::checkImagesDimensions(m_selectedMasks, m_patchSize, m_patchSize) &&
    // FileUtils::checkImagesHaveMasks(m_selectedImages, m_selectedMasks);

    return true;
}

void TrainModelWindow::onStartTrainingClicked()
{
    if (m_startTrainingBtn)
    {
        m_startTrainingBtn->set_sensitive(false); // Disable the button
        m_startTrainingBtn->set_label("Training..."); // Change the button text
    }

    m_modelPath = "";
    m_errorMsgLbl->set_text(Glib::ustring(""));

    // Validate training settings
    if (m_pyEnv.empty() || m_patchSize <= 0 || m_batchSize <= 0 || m_epochs <= 0)
    {
        if (m_errorMsgLbl)
        {
            m_errorMsgLbl->set_text(Glib::ustring("One or more training settings are not correctly configured."));
        }

        if (m_startTrainingBtn)
        {
            m_startTrainingBtn->set_sensitive(true);
            m_startTrainingBtn->set_label("Start");
        }
        return;
    }

    // Get current time and format it as YYYYMMDD_HHMMSS
    char timestamp[20];
    std::time_t now = std::time(nullptr);
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    std::string timestampStr(timestamp);
    std::string modelFolder = "model_" + timestampStr;
    std::string userDocs = Glib::get_user_special_dir(Glib::USER_DIRECTORY_DOCUMENTS);
    bool isModelDirCreated = FileUtils::createSubdirectory(userDocs, modelFolder);
    if (!isModelDirCreated)
    {
        std::cerr << "Failed to create 'model' directory. Exiting function." << std::endl;

        if (m_startTrainingBtn)
        {
            m_startTrainingBtn->set_sensitive(true);
            m_startTrainingBtn->set_label("Start");
        }
    }
    else
    {
        m_modelPath = Glib::build_filename(userDocs, modelFolder);
        std::filesystem::path trainImagesPath, trainMasksPath;
        std::filesystem::path valImagesPath, valMasksPath;
        std::filesystem::path testImagesPath, testMasksPath;

        if (!validateInputDataset())
        {
            if (m_errorMsgLbl)
            {
                m_errorMsgLbl->set_text(Glib::ustring("Error occurs when loading the selected dataset for training."));
            }

            if (m_startTrainingBtn)
            {
                m_startTrainingBtn->set_sensitive(true);
                m_startTrainingBtn->set_label("Start");
            }
        }
        else if (!FileUtils::createTrainingDatasetDirs(m_modelPath, trainImagesPath, trainMasksPath, valImagesPath, valMasksPath, testImagesPath, testMasksPath))
        {
            if (m_errorMsgLbl)
            {
                m_errorMsgLbl->set_text(Glib::ustring("Error occurs when creating training dataset directories."));
            }

            if (m_startTrainingBtn)
            {
                m_startTrainingBtn->set_sensitive(true);
                m_startTrainingBtn->set_label("Start");
            }
        }
        else if (!FileUtils::splitAndCopyImagesAndMasks(m_selectedImages, m_selectedMasks, m_modelPath, 0.8, 0.1, 0.1))
        {
            if (m_errorMsgLbl)
            {
                m_errorMsgLbl->set_text(Glib::ustring("Error occurs when preparing training dataset."));
            }

            if (m_startTrainingBtn)
            {
                m_startTrainingBtn->set_sensitive(true);
                m_startTrainingBtn->set_label("Start");
            }
        }
        else
        {
            // Set test imags and masks for testing the model
            m_testImagesPath = testImagesPath;
            m_testMasksPath = testMasksPath;

            // Command to execute the python script
            std::string cmd = m_pyEnv + std::string(" ../unet.py") +
                              std::string(" --model_path ") + Glib::build_filename(m_modelPath, "ds.keras") +
                              std::string(" --train_images_path ") + trainImagesPath.string() +
                              std::string(" --train_masks_path ") + trainMasksPath.string() +
                              std::string(" --val_images_path ") + valImagesPath.string() +
                              std::string(" --val_masks_path ") + valMasksPath.string() +
                              std::string(" --patch_size ") + std::to_string(m_patchSize) +
                              std::string(" --batch_size ") + std::to_string(m_batchSize) +
                              std::string(" --epochs ") + std::to_string(m_epochs);

            // Run the command in a separate thread
            std::thread([this, cmd]() {
                // Open a pipe to the command
                FILE *pipe = popen(cmd.c_str(), "r");
                if (!pipe)
                {
                    std::cerr << "Failed to run command\n";
                    return;
                }

                // Buffer to hold each line of output
                std::array<char, 128> buffer;

                // Read the output from the pipe line by line
                while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
                {
                    std::cout << buffer.data(); // Print each line to the console
                }

                // Close the pipe
                int returnCode = pclose(pipe);
                if (returnCode != 0)
                {
                    std::cerr << "Command failed with return code " << returnCode << std::endl;
                }

                // Optionally handle the result here or update the UI (make sure UI updates happen on the main thread)
                std::cout << "Python script finished execution." << std::endl;

                // Re-enable the button and reset the text back to "Start" on the main thread
                Glib::signal_idle().connect_once([this]() {
                    if (m_startTrainingBtn)
                    {
                        m_startTrainingBtn->set_sensitive(true);
                        m_startTrainingBtn->set_label("Start");
                    }
                });
            }).detach(); // Detach the thread so it runs independently
        }
    }
}

void TrainModelWindow::onViewModelClicked()
{
    if (m_modelPath.empty())
    {
        return;
    }

    std::string command = "xdg-open " + m_modelPath;
    if (std::system(command.c_str()) != 0) {
        std::cerr << "Failed to open directory." << std::endl;
    }
}

void TrainModelWindow::onTestModelClicked()
{
    if (m_testModelBtn)
    {
        m_testModelBtn->set_sensitive(false);
        m_testModelBtn->set_label("Testing...");
    }

    if (m_predFidelity <= 0)
    {
        if (m_testModelBtn)
        {
            m_testModelBtn->set_sensitive(true);
            m_testModelBtn->set_label("Test");
        }
        return;
    }
    
    // Command to execute the python script
    std::string cmd = m_pyEnv + std::string(" ../unet_test.py") +
                        std::string(" --model_path ") + Glib::build_filename(m_modelPath, "ds.keras") +
                        std::string(" --test_images_path ") + m_testImagesPath +
                        std::string(" --test_masks_path ") + m_testMasksPath +
                        std::string(" --patch_size ") + std::to_string(m_patchSize) +
                        std::string(" --batch_size ") + std::to_string(m_batchSize) +
                        std::string(" --threshold ") + std::to_string(m_predFidelity);

    // Run the command in a separate thread
    std::thread([this, cmd]() {
        // Open a pipe to the command
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Failed to run command\n";
        }

        // Buffer to hold each line of output
        std::array<char, 128> buffer;

        // Read the output from the pipe line by line
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            std::cout << buffer.data(); // Print each line to the console
        }

        // Close the pipe
        int returnCode = pclose(pipe);
        if (returnCode != 0)
        {
            std::cerr << "Command failed with return code " << returnCode << std::endl;
        }

        // Re-enable the button and reset the text back to "Start" on the main thread
        Glib::signal_idle().connect_once([this]() {
            if (m_testModelBtn)
            {
                m_testModelBtn->set_sensitive(true);
                m_testModelBtn->set_label("Test");
            }
        });
    }).detach(); // Detach the thread so it runs independently
}

void TrainModelWindow::onViewTestResultClicked()
{
    if (m_modelPath.empty())
    {
        return;
    }

    std::string command = "xdg-open " + Glib::build_filename(m_modelPath, "test_result");
    if (std::system(command.c_str()) != 0) {
        std::cerr << "Failed to open directory." << std::endl;
    }
}