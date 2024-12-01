#include "mainwindow.h"

const std::string MainWindow::SettingsFilePath = std::string(std::getenv("HOME")) + "/.config/deep-scan/settings.ini";

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frameQueue(20),
      m_logger(logger)
{
    // Create a CssProvider
    auto css_file = FileUtils::getCssFilePath();
    auto provider = Gtk::CssProvider::create();
    provider->load_from_path(css_file);
    
    // Apply the CSS provider to the default screen
    Gtk::StyleContext::add_provider_for_screen(
        Gdk::Screen::get_default(),
        provider,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );

    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("root", root);
    root->set_title("Deep Scan");

    // Top Menu
    m_builder->get_widget("main_stack", m_main_stack);
    m_builder->get_widget("menu_capture_rbtn", m_menu_capture_rbtn);
    if (m_menu_capture_rbtn)
    {
        m_menu_capture_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }
    m_builder->get_widget("menu_annotation_rbtn", m_menu_annotation_rbtn);
    if (m_menu_annotation_rbtn)
    {
        m_menu_annotation_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }
    m_builder->get_widget("menu_training_rbtn", m_menu_training_rbtn);
    if (m_menu_training_rbtn)
    {
        m_menu_training_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }
    m_builder->get_widget("menu_test_rbtn", m_menu_test_rbtn);
    if (m_menu_test_rbtn)
    {
        m_menu_test_rbtn->signal_toggled().connect(sigc::mem_fun(*this, &MainWindow::on_menu_toggled));
    }

    // Get the button by ID and connect the signal handler.
    m_builder->get_widget("discover_btn", m_discoverBtn);
    m_builder->get_widget("start_capture_btn", m_startCaptureBtn);
    m_builder->get_widget("stop_capture_btn", m_stopCaptureBtn);
    m_builder->get_widget("annotation_filter_btn", m_annotation_filter_btn);
    m_builder->get_widget("open_drawing_btn", m_openDrawingDialogBtn);
    m_builder->get_widget("open_patch_btn", m_openPatchDialogBtn);
    m_builder->get_widget("test_model_btn1", m_testModelBtn);

    // Disable buttons initially
    m_startCaptureBtn->set_sensitive(false);
    m_annotation_filter_btn->set_sensitive(false);
    m_openDrawingDialogBtn->set_sensitive(false);
    m_openPatchDialogBtn->set_sensitive(false);
    m_testModelBtn->set_sensitive(false);

    if (m_discoverBtn)
    {
        m_discoverBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDiscoverClicked));
    }
    if (m_startCaptureBtn)
    {
        m_startCaptureBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStartCaptureClicked));
    }
    if (m_stopCaptureBtn)
    {
        m_stopCaptureBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStopCaptureClicked));
    }
    if (m_annotation_filter_btn)
    {
        m_annotation_filter_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onFilterImagesClicked));
    }
    if (m_openDrawingDialogBtn)
    {
        m_openDrawingDialogBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenDrawingClicked));
    }
    if (m_openPatchDialogBtn)
    {
        m_openPatchDialogBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenPreprocessingClicked));
    }

    // Image Acquiring
    m_builder->get_widget("capture_source_cbox", m_cameraComboBox);
    if (m_cameraComboBox)
    {
        m_cameraComboBox->signal_changed().connect([this]()
                                                   {
            auto selectedCamera = m_cameraComboBox->get_active_text();
            auto folder = m_capturePickerFcb->get_filename();
            m_startCaptureBtn->set_sensitive(!folder.empty() && !selectedCamera.empty()); });
    }

    m_builder->get_widget("capture_picker_fcb", m_capturePickerFcb);
    if (m_capturePickerFcb)
    {
        m_capturePickerFcb->signal_selection_changed().connect([this]()
                                                               {
            auto selectedCamera = m_cameraComboBox->get_active_text();
            auto folder = m_capturePickerFcb->get_filename();
            m_startCaptureBtn->set_sensitive(!folder.empty() && !selectedCamera.empty()); });
    }
    m_builder->get_widget("capture_rate_sb", m_captureRateSb);

    m_builder->get_widget("filter_picker_fcb", m_filter_picker_fcb);
    if (m_filter_picker_fcb)
    {
        m_filter_picker_fcb->signal_selection_changed().connect([this]()
        {
            // Get the selected folder path
            std::string images_dir = m_filter_picker_fcb->get_filename();
            std::string py_env = "";

            if (m_annotation_py_env_entry)
            {
                py_env = m_annotation_py_env_entry->get_text();
            }

            m_annotation_filter_btn->set_sensitive(!images_dir.empty() && !py_env.empty()); 
        });
    }

    // Image Labelling
    m_builder->get_widget("labeling_picker_fcb", m_labelingPickerFcb);
    if (m_labelingPickerFcb)
    {
        m_labelingPickerFcb->signal_selection_changed().connect([this]()
        {
            // Get the selected folder path
            auto folder = m_labelingPickerFcb->get_filename();

            m_imageLabelingPath = folder;

            // Enable the start button if a folder is selected
            m_openDrawingDialogBtn->set_sensitive(!folder.empty());
        });
    }

    m_builder->get_widget("annotation_py_env_entry", m_annotation_py_env_entry);
    if (m_annotation_py_env_entry)
    {
        m_annotation_py_env_entry->signal_changed().connect([this]() 
        { 
            std::string py_env = m_annotation_py_env_entry->get_text();
            std::string images_dir = "";
            
            if (m_filter_picker_fcb)
            {
                images_dir = m_filter_picker_fcb->get_filename();
            }

            m_annotation_filter_btn->set_sensitive(!images_dir.empty() && !py_env.empty()); 
        });
    }

    m_builder->get_widget("patch_picker_fcb", m_patchPickerFcb);
    if (m_patchPickerFcb)
    {
        m_patchPickerFcb->signal_selection_changed().connect([this]()
        {
            // Get the selected folder path
            auto folder = m_patchPickerFcb->get_filename();

            m_preprocessImagePath = folder;

            // Enable the start button if a folder is selected
            m_openPatchDialogBtn->set_sensitive(!folder.empty());
        });
    }

    m_builder->get_widget("model_Images_picker_fcb", m_modelImagesPickerFcb);
    if (m_modelImagesPickerFcb)
    {
        m_modelImagesPickerFcb->signal_selection_changed().connect([this]()
        {
            m_trainModelImagesPath = m_modelImagesPickerFcb->get_filename();
        });
    }

    m_builder->get_widget("model_masks_picker_fcb", m_modelMasksPickerFcb);
    if (m_modelMasksPickerFcb)
    {
        m_modelMasksPickerFcb->signal_selection_changed().connect([this]()
        {
            m_trainModelMasksPath = m_modelMasksPickerFcb->get_filename();
        });
    }

    m_builder->get_widget("model_picker_fcb", m_modelPickerFcb);
    if (m_modelPickerFcb)
    {
        m_modelPickerFcb->signal_selection_changed().connect([this]()
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("test_images_picker_fcb", m_testImagesPickerFcb);
    if (m_testImagesPickerFcb)
    {
        m_testImagesPickerFcb->signal_selection_changed().connect([this]()
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("test_masks_picker_fcb", m_testMasksPickerFcb);
    if (m_testMasksPickerFcb)
    {
        m_testMasksPickerFcb->signal_selection_changed().connect([this]()
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("py_env_entry1", m_pyEnvEntry);
    if (m_pyEnvEntry)
    {
        m_pyEnvEntry->signal_changed().connect([this]() 
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("patch_size_sb", m_patchSizeSb);
    if (m_patchSizeSb)
    {
        m_patchSizeSb->signal_value_changed().connect([this]()
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("confidence_threshold_sb", m_confidenceThresholdSb);
    if (m_confidenceThresholdSb)
    {
        m_confidenceThresholdSb->signal_value_changed().connect([this]()
        { 
            auto model = m_modelPickerFcb->get_filename();
            auto testImages = m_testImagesPickerFcb->get_filename();
            auto testMasks = m_testMasksPickerFcb->get_filename();
            auto pyEnv = m_pyEnvEntry->get_text();
            auto patchSize = m_patchSizeSb->get_value();
            auto confidenceThreshold =  m_confidenceThresholdSb->get_value();
            m_testModelBtn->set_sensitive(!model.empty() && !testImages.empty() && !testMasks.empty() && !pyEnv.empty() && patchSize > 0 && confidenceThreshold > 0.0);
        });
    }
    m_builder->get_widget("test_model_btn1", m_testModelBtn);
    if (m_testModelBtn)
    {
        m_testModelBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onTestModelClicked));
    }
    m_builder->get_widget("view_test_result_lbtn1", m_viewTestResultBtn);
    if (m_viewTestResultBtn)
    {
        m_viewTestResultBtn->set_sensitive(false);
        m_viewTestResultBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onViewTestResultClicked));
    }
}

MainWindow::~MainWindow()
{
}

void MainWindow::on_menu_toggled()
{
    if (m_menu_capture_rbtn->get_active())
    {
        m_main_stack->set_visible_child("page_capture");
    }
    else if (m_menu_annotation_rbtn->get_active())
    {
        m_main_stack->set_visible_child("page_annotation");
    }
    else if (m_menu_training_rbtn->get_active())
    {
        m_main_stack->set_visible_child("page_training");
    }
    else if (m_menu_test_rbtn->get_active())
    {
        m_main_stack->set_visible_child("page_test");
    }    
}

std::string getIpV4AddressString(uint32_t ip)
{
    std::ostringstream ipStream;
    for (int i = 0; i < 4; ++i)
    {
        if (i > 0)
        {
            ipStream << ".";
        }
        ipStream << ((ip >> (24 - 8 * i)) & 0xFF);
    }
    return ipStream.str();
}

int getSelectedCamIndex(Gtk::TreeView *camTreeView, Glib::RefPtr<Gtk::ListStore> camsListStore)
{
    int index = 0;
    Glib::RefPtr<Gtk::TreeSelection> selection = camTreeView->get_selection();
    Gtk::TreeModel::iterator iter = selection->get_selected();
    if (iter)
    {
        Gtk::TreeModel::Children::iterator it;
        Gtk::TreeModel::Children children = camsListStore->children();

        for (it = children.begin(); it != children.end(); ++it)
        {
            if (it == iter)
            {
                return index;
            }
            ++index;
        }
    }
    return -1;
}

void saveImageAsync(FrameData frameData, void *deviceHandle, std::string folderPath)
{
    auto pData = frameData.pData;
    auto pMetadata = frameData.pMetadata;

    MV_SAVE_IMG_TO_FILE_PARAM stSaveFileParam;
    memset(&stSaveFileParam, 0, sizeof(MV_SAVE_IMG_TO_FILE_PARAM));

    stSaveFileParam.enImageType = MV_Image_Bmp;
    stSaveFileParam.enPixelType = pMetadata->enPixelType;
    stSaveFileParam.nWidth = pMetadata->nWidth;
    stSaveFileParam.nHeight = pMetadata->nHeight;
    stSaveFileParam.nDataLen = pMetadata->nFrameLen;
    stSaveFileParam.pData = pData;

    //sprintf(stSaveFileParam.pImagePath, "%sImage_ts%d_fn%d.bmp", folderPath.c_str(), pMetadata->nHostTimeStamp, pMetadata->nFrameNum);
    sprintf(stSaveFileParam.pImagePath, "%sImage_ts%" PRId64 "_fn%d_dv%" PRIuPTR ".bmp", 
            folderPath.c_str(), pMetadata->nHostTimeStamp, pMetadata->nFrameNum, 
            reinterpret_cast<uintptr_t>(deviceHandle));

    int nRet = MV_CC_SaveImageToFile(deviceHandle, &stSaveFileParam);
    if (nRet != MV_OK)
    {
        std::cout << "Failed to save image to file. Error code: " << nRet << std::endl;
    }
}

void MainWindow::onDiscoverClicked()
{
    m_cameraComboBox->remove_all();

    memset(&m_camList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    // enum device
    int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &m_camList);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_EnumDevices: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    if (m_camList.nDeviceNum > 0)
    {
        for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
        {
            MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
            if (NULL == pDeviceInfo)
            {
                return;
            }

            if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
            {
                auto modelName = pDeviceInfo->SpecialInfo.stGigEInfo.chModelName;
                auto friendlyName = pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName;
                auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;

                // Add the camera name to the combo box
                m_cameraComboBox->append(std::string((char *)serialNumber));
            }
        }
        m_cameraComboBox->append("All Cameras");
    }
    else
    {
        std::cout << "No device found." << std::endl;
        m_logger->log("No device found.");
    }
}

void *MainWindow::getDeviceHandleBySerialNumber(std::string sn)
{
    for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
            std::string serialNumberStr(reinterpret_cast<const char *>(serialNumber));
            if (serialNumberStr == sn)
            {
                void *deviceHandle;
                int nRet = MV_CC_CreateHandle(&deviceHandle, pDeviceInfo);
                if (nRet != MV_OK)
                {
                    std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                    m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
                    return nullptr;
                }
                return deviceHandle;
            }
        }
    }

    return nullptr;
}

std::vector<void*> MainWindow::getAllDeviceHandles()
{
    std::vector<void*> deviceHandles;  // To store all device handles

    for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
        if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            void *deviceHandle;
            int nRet = MV_CC_CreateHandle(&deviceHandle, pDeviceInfo);
            if (nRet != MV_OK)
            {
                std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
                continue;  // Skip this device if handle creation failed
            }

            deviceHandles.push_back(deviceHandle);  // Add handle to the list
        }
    }

    return deviceHandles;  // Return all device handles
}

void MainWindow::preflight(void *deviceHandle)
{
    // Connect to the device
    int nRet = MV_CC_OpenDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    m_logger->log("Connected to device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Detect network optimal packet size(It only works for the GigE camera)
    int nPacketSize = MV_CC_GetOptimalPacketSize(deviceHandle);
    if (nPacketSize > 0)
    {
        nRet = MV_CC_SetIntValue(deviceHandle, "GevSCPSPacketSize", nPacketSize);
        if (nRet != MV_OK)
        {
            std::cout << "Set Packet Size fail. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(GevSCPSPacketSize): " + std::to_string(nRet), Logger::ERROR);
        }
    }
    else
    {
        std::cout << "Get Packet Size fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetOptimalPacketSize: " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("MV_CC_SetIntValue(GevSCPSPacketSize) for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Enable trigger mode
    nRet = MV_CC_SetEnumValue(deviceHandle, "TriggerMode", 1);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerMode fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetTriggerMode: " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("MV_CC_SetEnumValue(TriggerMode) for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Set trigger source
    nRet = MV_CC_SetEnumValue(deviceHandle, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerSource fail! Error code:" << nRet << std::endl;
        m_logger->log("Error on MV_CC_SetEnumValue(TriggerSource): " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("MV_CC_SetEnumValue(TriggerSource) for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));
}

void MainWindow::startCapture(void *deviceHandle, double captureIntervalMs, std::string captureDestFolder)
{
    m_isCapturing = true;

    // Register image callback
    auto imageCaptureCallback = [](unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser)
    {
        if (pFrameInfo)
        {
            std::cout << "GetOneFrame, nDevTimeStampHigh: " << pFrameInfo->nDevTimeStampHigh
                    << ", nDevTimeStampLow: " << pFrameInfo->nDevTimeStampLow
                    << ", nHostTimeStamp: " << pFrameInfo->nHostTimeStamp
                    << std::endl;
        }

        // Cast pUser to MainWindow*
        MainWindow *pThis = static_cast<MainWindow *>(pUser);

        pThis->m_frameQueue.enqueue(FrameData(pData, pFrameInfo));
    };

    int nRet = MV_CC_RegisterImageCallBackEx(deviceHandle, imageCaptureCallback, this);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_RegisterImageCallBackEx fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_RegisterImageCallBackEx: " + std::to_string(nRet), Logger::ERROR);
        return;
    }
    m_logger->log("MV_CC_RegisterImageCallBackEx for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Process images in a separate thread
    auto processFrameAsync = [this, deviceHandle, captureDestFolder]()
    {
        // Ensure that the folder path ends with a slash
        std::string filePath = captureDestFolder;
        if (!filePath.empty() && filePath.back() != '/')
        {
            filePath += '/';
        }

        FrameData frameData(nullptr, nullptr); // Initialize FrameData with null pointers

        while (m_isCapturing)
        {
            if (m_frameQueue.dequeue(frameData))
            {
                // Save image async
                std::async(std::launch::async, saveImageAsync, frameData, deviceHandle, filePath);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            }
        }

        // Flush frame queue
        while (!m_frameQueue.isEmpty())
        {
            if (m_frameQueue.dequeue(frameData))
            {
                // Save image async
                std::async(std::launch::async, saveImageAsync, frameData, deviceHandle, filePath);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            }
        }
    };

    // Create and start the frame processing thread
    std::thread processingThread(processFrameAsync);
    m_logger->log("Started frame processing thread for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Detach the thread to let it run in the background and won't be able to join later
    processingThread.detach();

    // Capture images in a separate thread
    auto captureFrameAsync = [this, deviceHandle, captureIntervalMs]()
    {
        uint64_t lastCaptureTimestamp = 0;

        while (m_isCapturing)
        {
            auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            double elapsed = currentTimeInMs - lastCaptureTimestamp;

            if (elapsed >= captureIntervalMs)
            {
                lastCaptureTimestamp = currentTimeInMs;

                int nRet = MV_CC_SetCommandValue(deviceHandle, "TriggerSoftware");
                if (MV_OK != nRet)
                {
                    std::cout << "Failed to capture frames via TriggerSoftware. Error code: " << nRet << std::endl;
                    m_logger->log("Error on MV_CC_SetCommandValue(TriggerSoftware): " + std::to_string(nRet), Logger::ERROR);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Prevent CPU overuse
        }
    };

    // Create and start the frame acquisition thread
    std::thread capturingThread(captureFrameAsync);
    m_logger->log("Started frame capturing thread for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

    // Detach the thread to let it run in the background and won't be able to join later
    capturingThread.detach();

    // Start grab images
    nRet = MV_CC_StartGrabbing(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StartGrabbing: " + std::to_string(nRet), Logger::ERROR);
    }
    m_logger->log("Started MV_CC_StartGrabbing for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));
}

void MainWindow::onStartCaptureClicked()
{
    // Reset device handles
    if (!m_deviceHandles.empty())
    {
        m_deviceHandles.clear();
    }

    // Disable start capture btn
    m_startCaptureBtn->set_sensitive(false);

    double captureIntervalMs = 0.0;
    std::string captureDestFolder;
    uint64_t captureElapsedTime = 0;

    if (m_cameraComboBox)
    {
        auto selectedCaptureDevice = m_cameraComboBox->get_active_text();
        if (selectedCaptureDevice == "All Cameras")
        {
            m_deviceHandles = getAllDeviceHandles();
        }
        else
        {
            // Create device handle
            auto deviceHandle = getDeviceHandleBySerialNumber(selectedCaptureDevice);
            if (deviceHandle == nullptr)
            {
                std::cout << "getDeviceHandleBySerialNumber fail! deviceHandle is nullptr" << std::endl;
                m_logger->log("Error on getDeviceHandleBySerialNumber: deviceHandle is nullptr.", Logger::ERROR);
            }
            m_deviceHandles.push_back(deviceHandle);
        }
    }

    if (m_capturePickerFcb)
    {
        captureDestFolder = m_capturePickerFcb->get_filename();

        // Get current time and format it as YYYYMMDD_HHMMSS
        char timestamp[20];
        std::time_t now = std::time(nullptr);
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));

        // Append timestamp to the folder path
        std::string timestampStr(timestamp);
        captureDestFolder += "/" + timestampStr;

        // Create the subfolder if it doesn't exist
        try
        {
            if (!std::filesystem::exists(captureDestFolder))
            {
                std::filesystem::create_directory(captureDestFolder);
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error creating the directory: " << e.what() << std::endl;
            m_logger->log("Error creating the directory:" + std::string(e.what()), Logger::ERROR);
        }
    }

    if (m_captureRateSb)
    {
        int captureRate = m_captureRateSb->get_value();
        // Calculate the capture interval (in milliseconds) based on capture rate (FPS)
        captureIntervalMs = 1000.0 / static_cast<double>(captureRate);
    }

    // Preflight
    for (void *deviceHandle : m_deviceHandles)
    {
        auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "Preflight request to device: " << deviceHandle << " at " << currentTimeInMs << std::endl;
        m_logger->log("Preflight request to device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));

        // Launch startCapture asynchronously for each device
        std::async(std::launch::async, &MainWindow::preflight, this, deviceHandle);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Start capturing
    for (void *deviceHandle : m_deviceHandles)
    {
        auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        std::cout << "Begin capture for device: " << deviceHandle << " at " << currentTimeInMs << std::endl;
        m_logger->log("Begin capture");

        // Launch startCapture asynchronously for each device
        std::async(std::launch::async, &MainWindow::startCapture, this, deviceHandle, captureIntervalMs, captureDestFolder);
    }
}

void MainWindow::onStopCaptureClicked()
{
    for (void *deviceHandle : m_deviceHandles)
    {
        m_logger->log("Stop capture for device: " + std::to_string(reinterpret_cast<uintptr_t>(deviceHandle)));
        // Launch stopCapture asynchronously for each device
        std::async(std::launch::async, &MainWindow::stopCapture, this, deviceHandle);
    }

    if (!m_deviceHandles.empty())
    {
        m_deviceHandles.clear();
    }

    // Enable start capture btn
    m_startCaptureBtn->set_sensitive(true);
}

void MainWindow::stopCapture(void *deviceHandle)
{
    m_isCapturing = false;

    int nRet = MV_CC_StopGrabbing(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_StopGrabbing", Logger::ERROR);
    }

    // Close the device
    nRet = MV_CC_CloseDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
    }

    // Destory the device handle
    nRet = MV_CC_DestroyHandle(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::onFilterImagesClicked()
{
    if (m_annotation_filter_btn)
    {
        m_annotation_filter_btn->set_sensitive(false); // Disable the button
        m_annotation_filter_btn->set_label("Filtering...");
    }

    // Write the Python script to the temp file
    std::string tempPyPath = "/tmp/deep_scan/temp_mobilenet_v2.py";
    if (!FileUtils::createSubdirectory("/tmp", "deep_scan"))
    {
        std::cerr << "Failed to create tmp directory.";
        m_logger->log("Unable to create tmp directory: /tmp/deep_scan", Logger::ERROR);

        if (m_annotation_filter_btn)
        {
            m_annotation_filter_btn->set_sensitive(true);
            m_annotation_filter_btn->set_label("Filter");
        }

        return;
    }
    else
    {
        std::ofstream temp_mobilenet_v2_test(tempPyPath);
        if (temp_mobilenet_v2_test.is_open())
        {
            temp_mobilenet_v2_test << mobilenet_v2_test;
            temp_mobilenet_v2_test.close();
        }
        else
        {
            std::cerr << "Failed to open temp_mobilenet_v2_test.py for writing" << std::endl;
            m_logger->log("Unable to open temp_mobilenet_v2_test.py for writing", Logger::ERROR);

            if (m_annotation_filter_btn)
            {
                m_annotation_filter_btn->set_sensitive(true);
                m_annotation_filter_btn->set_label("Filter");
            }

            return;
        }
    }

    // Command to execute the python script
    std::string py_env = m_annotation_py_env_entry->get_text();
    std::string images_dir = m_filter_picker_fcb->get_filename();

    if (images_dir.empty() || py_env.empty())
    {
        if (m_annotation_filter_btn)
        {
            m_annotation_filter_btn->set_sensitive(true);
            m_annotation_filter_btn->set_label("Filter");
        }

        return;
    }

    std::string cmd = py_env + " " + tempPyPath + std::string(" --images_dir ") + images_dir;
    
    // Run the command in a separate thread
    std::thread([this, cmd, tempPyPath]() {
        // Open a pipe to the command
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Failed to open a pipe and run command\n";
            m_logger->log("Error to open a pipe and run command: " + cmd, Logger::ERROR);
            return;
        }

        // Buffer to hold each line of output
        std::array<char, 256> buffer;

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
            m_logger->log("Error to close the pipe: " + std::to_string(returnCode), Logger::ERROR);
        }

        // Optionally handle the result here or update the UI (make sure UI updates happen on the main thread)
        std::cout << "Python script finished execution." << std::endl;

        // Delete the tmp script after execution
        std::remove(tempPyPath.c_str());

        // Re-enable the button and reset the text on the main thread
        Glib::signal_idle().connect_once([this]() {
            if (m_annotation_filter_btn)
            {
                m_annotation_filter_btn->set_sensitive(true);
                m_annotation_filter_btn->set_label("Filter");
            }
        });
    }).detach(); // Detach the thread so it runs independently
}

void MainWindow::onOpenDrawingClicked()
{
    // Create the DrawWindow from the Glade file
    auto gladeFile = FileUtils::getGladeFilePath();
    DrawWindow *drawWindow = DrawWindow::create(gladeFile);

    if (drawWindow)
    {
        drawWindow->setImageLabelingPath(m_imageLabelingPath);
        drawWindow->present(); // Show the window
    }
}

void MainWindow::onOpenPreprocessingClicked()
{
    auto gladeFile = FileUtils::getGladeFilePath();
    PreprocessWindow *PreprocessWindow = PreprocessWindow::create(gladeFile);

    if (PreprocessWindow)
    {
        PreprocessWindow->setImagePreprocessingPath(m_preprocessImagePath);
        PreprocessWindow->present(); // Show the window
    }
}

void MainWindow::onTestModelClicked()
{
    m_testModelBtn->set_sensitive(false);
    m_testModelBtn->set_label("Testing...");

    if (m_viewTestResultBtn)
    {
        m_viewTestResultBtn->set_sensitive(false);
    }

    auto modelPath = m_modelPickerFcb->get_filename();
    auto testImages = m_testImagesPickerFcb->get_filename();
    auto testMasks = m_testMasksPickerFcb->get_filename();
    auto pyEnv = m_pyEnvEntry->get_text();
    auto patchSize = static_cast<int>(m_patchSizeSb->get_value());
    auto confidenceThreshold =  m_confidenceThresholdSb->get_value();

    // Write the Python script to the temp file
    std::string tempPyPath = "/tmp/deep_scan/temp_unet_test.py";
    if (!FileUtils::createSubdirectory("/tmp", "deep_scan"))
    {
        std::cerr << "Failed to create tmp directory.";

        if (m_testModelBtn)
        {
            m_testModelBtn->set_sensitive(true);
            m_testModelBtn->set_label("Start");
        }

        return;
    }
    else
    {
        std::ofstream tempUnetTestPyFile(tempPyPath);
        if (tempUnetTestPyFile.is_open())
        {
            tempUnetTestPyFile << unet_test_py;
            tempUnetTestPyFile.close();
        }
        else
        {
            std::cerr << "Failed to open temp_unet.py for writing" << std::endl;
            m_logger->log("Unable to open temp_unet.py for writing", Logger::ERROR);

            if (m_testModelBtn)
            {
                m_testModelBtn->set_sensitive(true);
                m_testModelBtn->set_label("Start");
            }

            return;
        }
    }

    // Command to execute the python script
    std::string cmd = pyEnv + " " + tempPyPath +
                        std::string(" --model_path ") + modelPath +
                        std::string(" --test_images_path ") + testImages +
                        std::string(" --test_masks_path ") + testMasks +
                        std::string(" --patch_size ") + std::to_string(patchSize) +
                        std::string(" --batch_size 16") +
                        std::string(" --threshold ") + std::to_string(confidenceThreshold);

    // Run the command in a separate thread
    std::thread([this, cmd, tempPyPath, modelPath]() {
        // Open a pipe to the command
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Failed to run command\n";
            m_logger->log("Unable to run command: " + cmd, Logger::ERROR);
        }

        // Open log file for writing
        std::filesystem::path path(modelPath);
        auto modelDir = path.parent_path().string();
        std::string logFilePath = Glib::build_filename(modelDir, "ds.log");
        std::ofstream logFile(logFilePath, std::ios::out | std::ios::app); // Append mode
        if (logFile.is_open())
        {
            std::string cmdForLogging = cmd;
            // Find and replace tempPyPath with "ds_test.py" to hide the actual model py test script
            size_t pos = cmdForLogging.find(tempPyPath);
            if (pos != std::string::npos) 
            {
                cmdForLogging.replace(pos, tempPyPath.length(), "ds_test.py");
            }
            logFile << cmdForLogging << std::endl;
        }

        // Buffer to hold each line of output
        std::array<char, 128> buffer;

        // Read the output from the pipe line by line
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr)
        {
            std::cout << buffer.data(); // Print each line to the console
            if (logFile.is_open()) {
                logFile << buffer.data(); // Write each line to the log file
            }
        }

        // Close the log file
        if (logFile.is_open()) {
            logFile.close();
        }

        // Close the pipe
        int returnCode = pclose(pipe);
        if (returnCode != 0)
        {
            std::cerr << "Command failed with return code " << returnCode << std::endl;
            m_logger->log("Unable to close the pipe: " + std::to_string(returnCode), Logger::ERROR);
        }

        // Delete the tmp script after execution
        std::remove(tempPyPath.c_str());

        // Re-enable the button and reset the text back to "Start" on the main thread
        Glib::signal_idle().connect_once([this]() {
            if (m_testModelBtn)
            {
                m_testModelBtn->set_sensitive(true);
                m_testModelBtn->set_label("Test");
            }

            if (m_viewTestResultBtn)
            {
                m_viewTestResultBtn->set_sensitive(true);
            }
        });
    }).detach(); // Detach the thread so it runs independently
}

void MainWindow::onViewTestResultClicked()
{
    auto modelPath = m_modelPickerFcb->get_filename();

    if (modelPath.empty())
    {
        return;
    }

    std::filesystem::path path(modelPath);
    auto modelDir = path.parent_path().string();
    std::string command = "xdg-open " + Glib::build_filename(modelDir, "test_result");
    if (std::system(command.c_str()) != 0)
    {
        std::cerr << "Failed to open directory." << std::endl;
        m_logger->log("Error to open the directory via command: " + command, Logger::ERROR);
    }
}