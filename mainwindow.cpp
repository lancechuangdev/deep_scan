#include "mainwindow.h"

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_captureDuration(5),
      m_captureInterval(0),
      m_lastCaptureTimestamp(0),
      m_frameQueue(20),
      m_running(false)
{
    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("root", root);
    root->set_title("Deep Scan");

    // Get the button by ID and connect the signal handler.
    m_builder->get_widget("discover_btn", m_discoverBtn);
    m_builder->get_widget("connect_btn", m_connectBtn);
    m_builder->get_widget("start_btn", m_startBtn);
    m_builder->get_widget("stop_btn", m_stopBtn);
    m_builder->get_widget("disconnect_btn", m_disconnectBtn);
    m_builder->get_widget("open_drawing_btn", m_openDrawingDialogBtn);
    m_builder->get_widget("open_patch_btn", m_openPatchDialogBtn);
    m_builder->get_widget("open_training_btn", m_openTrainingDialogBtn);

    // Disable the start button initially
    m_connectBtn->set_sensitive(false);
    m_startBtn->set_sensitive(false);
    m_openDrawingDialogBtn->set_sensitive(false);
    m_openPatchDialogBtn->set_sensitive(false);
    m_openTrainingDialogBtn->set_sensitive(false);

    if (m_discoverBtn)
    {
        m_discoverBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDiscoverClicked));
    }
    if (m_connectBtn)
    {
        m_connectBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onConnectClicked));
    }
    if (m_startBtn)
    {
        m_startBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStartClicked));
    }
    if (m_stopBtn)
    {
        m_stopBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStopClicked));
    }
    if (m_disconnectBtn)
    {
        m_disconnectBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDisconnectClicked));
    }
    if (m_openDrawingDialogBtn)
    {
        m_openDrawingDialogBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenDrawingClicked));
    }
    if (m_openPatchDialogBtn)
    {
        m_openPatchDialogBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenPreprocessingClicked));
    }
    if (m_openTrainingDialogBtn)
    {
        m_openTrainingDialogBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onOpenTrainingClicked));
    }

    m_builder->get_widget("camera_list", m_camTreeView);

    // Connect the signal to a handler function
    Glib::RefPtr<Gtk::TreeSelection> selection = m_camTreeView->get_selection();
    selection->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::onTreeviewSelectionChanged));

    // Create the ListStore, with 'm_camcols' as the column model
    m_camListStore = Gtk::ListStore::create(m_camcols);

    // Set the ListStore as the model for the cams TreeView
    m_camTreeView->set_model(m_camListStore);

    // Append columns to the TreeView
    m_camTreeView->append_column("Model", m_camcols.col_model);
    m_camTreeView->append_column("Friendly Name", m_camcols.col_friendly_name);
    m_camTreeView->append_column("IP Address", m_camcols.col_ip);

    // Device settings
    m_builder->get_widget("exposure_lbl", m_exposureTimeLbl);
    m_builder->get_widget("frame_rate_lbl", m_frameRateLbl);
    m_builder->get_widget("width_lbl", m_widthLbl);
    m_builder->get_widget("height_lbl", m_heightLbl);
    m_builder->get_widget("gain_lbl", m_gainLbl);

    // Image Acquiring
    m_builder->get_widget("capture_picker_fcb", m_capturePickerFcb);
    if (m_capturePickerFcb)
    {
        m_capturePickerFcb->signal_selection_changed().connect([this]()
                                                               {
            // Get the selected folder path
            auto folder = m_capturePickerFcb->get_filename();

            // Enable the start button if a folder is selected
            m_startBtn->set_sensitive(!folder.empty()); });
    }
    m_builder->get_widget("capture_duration_sb", m_captureDurationSb);
    m_builder->get_widget("capture_rate_sb", m_captureRateSb);
    m_builder->get_widget("capture_pb", m_capturePb);

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
            m_openTrainingDialogBtn->set_sensitive(!m_trainModelImagesPath.empty() && !m_trainModelMasksPath.empty());
        });
    }

    m_builder->get_widget("model_masks_picker_fcb", m_modelMasksPickerFcb);
    if (m_modelMasksPickerFcb)
    {
        m_modelMasksPickerFcb->signal_selection_changed().connect([this]()
        {
            m_trainModelMasksPath = m_modelMasksPickerFcb->get_filename();
            m_openTrainingDialogBtn->set_sensitive(!m_trainModelImagesPath.empty() && !m_trainModelMasksPath.empty());
        });
    }
}

MainWindow::~MainWindow()
{
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

void MainWindow::onTreeviewSelectionChanged()
{
    int nIndex = getSelectedCamIndex(m_camTreeView, m_camListStore);
    if (nIndex < 0)
    {
        std::cout << "No camera was selected." << std::endl;
        m_connectBtn->set_sensitive(false);
    }
    else
    {
        MV_CC_DEVICE_INFO *pSelectedCam = m_camList.pDeviceInfo[nIndex];
        int nRet = MV_CC_CreateHandle(&m_selectedCam, pSelectedCam);
        if (nRet != MV_OK)
        {
            std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
        }
        else
        {
            m_connectBtn->set_sensitive(true);
        }
    }
}

void MainWindow::onDiscoverClicked()
{
    // Clear the TreeView before adding new data
    m_camListStore->clear();

    do
    {
        memset(&m_camList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

        // enum device
        int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &m_camList);
        if (nRet != MV_OK)
        {
            std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
            break;
        }

        if (m_camList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
            {
                std::cout << "device: " << i << std::endl;
                MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }

                Gtk::TreeModel::Row row = *(m_camListStore->append());

                if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
                {
                    row[m_camcols.col_model] = Glib::ustring(reinterpret_cast<const char *>(pDeviceInfo->SpecialInfo.stGigEInfo.chModelName));
                    row[m_camcols.col_friendly_name] = Glib::ustring(reinterpret_cast<const char *>(pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName));
                    row[m_camcols.col_ip] = getIpV4AddressString(pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp);
                }
            }
        }
        else
        {
            std::cout << "No device found." << std::endl;
            break;
        }
    } while (false);
}

void MainWindow::onConnectClicked()
{
    int nIndex = getSelectedCamIndex(m_camTreeView, m_camListStore);

    // Create device handler if needed
    if (!m_selectedCam)
    {
        if (nIndex < 0)
        {
            std::cout << "No camera was selected." << std::endl;
            return;
        }
        else
        {
            MV_CC_DEVICE_INFO *pSelectedCam = m_camList.pDeviceInfo[nIndex];
            int nRet = MV_CC_CreateHandle(&m_selectedCam, pSelectedCam);
            if (nRet != MV_OK)
            {
                std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
                return;
            }
        }
    }

    // Connect device
    int nRet = MV_CC_OpenDevice(m_selectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        return;
    }

    // Detect network optimal package size(It only works for the GigE camera)
    if (m_camList.pDeviceInfo[nIndex]->nTLayerType == MV_GIGE_DEVICE)
    {
        int nPacketSize = MV_CC_GetOptimalPacketSize(m_selectedCam);
        if (nPacketSize > 0)
        {
            nRet = MV_CC_SetIntValue(m_selectedCam, "GevSCPSPacketSize", nPacketSize);
            if (nRet != MV_OK)
            {
                std::cout << "Set Packet Size fail. Error code: " << nRet << std::endl;
            }
        }
        else
        {
            std::cout << "Get Packet Size fail. Error code: " << nRet << std::endl;
        }
    }

    // Enable trigger mode
    nRet = MV_CC_SetEnumValue(m_selectedCam, "TriggerMode", 1);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerMode fail! Error code: " << nRet << std::endl;
    }

    // Set trigger source
    nRet = MV_CC_SetEnumValue(m_selectedCam, "TriggerSource", MV_TRIGGER_SOURCE_SOFTWARE);
    if (MV_OK != nRet)
    {
        std::cout << "MV_CC_SetTriggerSource fail! Error code:" << nRet << std::endl;
    }

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

    nRet = MV_CC_RegisterImageCallBackEx(m_selectedCam, imageCaptureCallback, this);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_RegisterImageCallBackEx fail. Error code: " << nRet << std::endl;
        return;
    }

    populateDeviceSettings();
}

void MainWindow::populateDeviceSettings()
{
    // Exposure time
    MVCC_FLOATVALUE exposureTime = {0};
    int nRet = MV_CC_GetFloatValue(m_selectedCam, "ExposureTime", &exposureTime);
    if (MV_OK == nRet && m_exposureTimeLbl)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << exposureTime.fCurValue;

        // Set the label text
        m_exposureTimeLbl->set_text(Glib::ustring(oss.str()));
    }
    else
    {
        std::cout << "Failed to get exposure time. Error code: " << nRet << std::endl;
    }

    // Resulting Frame Rate
    MVCC_FLOATVALUE frameRate = {0};
    nRet = MV_CC_GetFloatValue(m_selectedCam, "ResultingFrameRate", &frameRate);
    if (MV_OK == nRet && m_frameRateLbl)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << frameRate.fCurValue;

        // Set the label text
        m_frameRateLbl->set_text(oss.str());
    }
    else
    {
        std::cout << "Failed to get frame rate. Error code: " << nRet << std::endl;
    }

    // Width
    MVCC_INTVALUE width = {0};
    nRet = MV_CC_GetIntValue(m_selectedCam, "Width", &width);
    if (MV_OK == nRet && m_widthLbl)
    {
        m_widthLbl->set_text(std::to_string(width.nCurValue));
    }
    else
    {
        std::cout << "Failed to get width. Error code: " << nRet << std::endl;
    }

    // Height
    MVCC_INTVALUE height = {0};
    nRet = MV_CC_GetIntValue(m_selectedCam, "Height", &height);
    if (MV_OK == nRet && m_heightLbl)
    {
        m_heightLbl->set_text(std::to_string(height.nCurValue));
    }
    else
    {
        std::cout << "Failed to get height. Error code: " << nRet << std::endl;
    }

    // Gain
    MVCC_FLOATVALUE gain = {0};
    nRet = MV_CC_GetFloatValue(m_selectedCam, "Gain", &gain);
    if (MV_OK == nRet && m_gainLbl)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << gain.fCurValue;

        // Set the label text
        m_gainLbl->set_text(oss.str());
    }
    else
    {
        std::cout << "Failed to get gain. Error code: " << nRet << std::endl;
    }
}

void MainWindow::clearDeviceSettings()
{
    if (m_exposureTimeLbl)
    {
        m_exposureTimeLbl->set_text(std::string());
    }
    if (m_frameRateLbl)
    {
        m_frameRateLbl->set_text(std::string());
    }
    if (m_widthLbl)
    {
        m_widthLbl->set_text(std::string());
    }
    if (m_heightLbl)
    {
        m_heightLbl->set_text(std::string());
    }
    if (m_gainLbl)
    {
        m_gainLbl->set_text(std::string());
    }
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

    sprintf(stSaveFileParam.pImagePath, "%sImage_w%d_h%d_fn%d.bmp", folderPath.c_str(), stSaveFileParam.nWidth, stSaveFileParam.nHeight, pMetadata->nFrameNum);

    int nRet = MV_CC_SaveImageToFile(deviceHandle, &stSaveFileParam);
    if (nRet != MV_OK)
    {
        std::cout << "Failed to save image to file. Error code: " << nRet << std::endl;
    }
}

void MainWindow::onStartClicked()
{
    m_running = true;

    if (m_capturePickerFcb)
    {
        m_imageFolderPath = m_capturePickerFcb->get_filename();

        // Get current time and format it as YYYYMMDD_HHMMSS
        char timestamp[20];
        std::time_t now = std::time(nullptr);
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&now));

        // Append timestamp to the folder path
        std::string timestampStr(timestamp);
        m_imageFolderPath += "/" + timestampStr;

        // Create the subfolder if it doesn't exist
        try
        {
            if (!std::filesystem::exists(m_imageFolderPath))
            {
                std::filesystem::create_directory(m_imageFolderPath);
            }
        }
        catch (const std::filesystem::filesystem_error &e)
        {
            std::cerr << "Error creating directory: " << e.what() << std::endl;
        }
    }
    if (m_captureDurationSb)
    {
        m_captureDuration = m_captureDurationSb->get_value();
    }
    if (m_captureRateSb)
    {
        int captureRate = m_captureRateSb->get_value();
        // Calculate the capture interval (in milliseconds) based on capture rate (FPS)
        m_captureInterval = 1000.0 / static_cast<double>(captureRate);
    }

    // Initialize progress bar
    m_capturePb->set_fraction(0.0); // Start at 0%
    m_captureElapsedTime = 0;

    // Start the timeout for the progress bar update using a lambda function
    m_captureTimeoutConnection = Glib::signal_timeout().connect(
        [this]() -> bool
        {
            m_captureElapsedTime += 100; // Increase the elapsed time by 100 ms

            // Calculate the capture duration in milliseconds
            auto duration = m_captureDuration * 60 * 1000;
            double fraction = static_cast<double>(m_captureElapsedTime) / duration;
            m_capturePb->set_fraction(fraction);

            if (m_captureElapsedTime >= duration)
            {
                m_running = false;

                // Time's up, stop the capturing and reset the progress bar
                int nRet = MV_CC_StopGrabbing(m_selectedCam);
                if (nRet != MV_OK)
                {
                    std::cout << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
                }
                m_capturePb->set_fraction(1.0);

                return false; // Return false to stop the timeout
            }

            return true; // Continue the timeout
        },
        100 // Update every 100 milliseconds
    );

    // Process images in a separate thread
    auto processFrameAsync = [this]()
    {
        // Ensure that the folder path ends with a slash
        std::string folderPath = m_imageFolderPath;
        if (!folderPath.empty() && folderPath.back() != '/')
        {
            folderPath += '/';
        }

        FrameData frameData(nullptr, nullptr); // Initialize FrameData with null pointers

        while (m_running)
        {
            if (m_frameQueue.dequeue(frameData))
            {
                // Save image async
                std::async(std::launch::async, saveImageAsync, frameData, m_selectedCam, folderPath);
            }
            else
            {
                std::cout << "No frame in the queue" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
            }
        }
    };

    // Create and start the frame processing thread
    std::thread processingThread(processFrameAsync);

    // Detach the thread to let it run in the background and won't be able to join later
    processingThread.detach();

    // Capture images in a separate thread
    auto captureFrameAsync = [this]()
    {
        while (m_running)
        {
            auto currentTimeInMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            double elapsed = currentTimeInMs - m_lastCaptureTimestamp;

            if (elapsed >= m_captureInterval)
            {
                std::cout << "Start to capture frames at time: " << currentTimeInMs << std::endl;
                m_lastCaptureTimestamp = currentTimeInMs;

                int nRet = MV_CC_SetCommandValue(m_selectedCam, "TriggerSoftware");
                if (MV_OK != nRet)
                {
                    std::cout << "Failed to capture frames via TriggerSoftware. Error code: " << nRet << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Prevent CPU overuse
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Prevent CPU overuse
        }
    };

    // Create and start the frame acquisition thread
    std::thread capturingThread(captureFrameAsync);

    // Detach the thread to let it run in the background and won't be able to join later
    capturingThread.detach();

    // Start grab images
    int nRet = MV_CC_StartGrabbing(m_selectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StartGrabbing fail. Error code: " << nRet << std::endl;
    }
}

void MainWindow::onStopClicked()
{
    m_running = false;

    // Stop the timeout
    if (m_captureTimeoutConnection.connected())
    {
        m_captureTimeoutConnection.disconnect();
    }

    int nRet = MV_CC_StopGrabbing(m_selectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_StopGrabbing fail. Error code: " << nRet << std::endl;
    }
}

void MainWindow::onDisconnectClicked()
{
    int nRet = MV_CC_CloseDevice(m_selectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
    }

    // destroy handle
    nRet = MV_CC_DestroyHandle(m_selectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
    }

    m_selectedCam = nullptr;

    clearDeviceSettings();
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

void MainWindow::onOpenTrainingClicked()
{
    auto gladeFile = FileUtils::getGladeFilePath();
    TrainModelWindow *TrainModelWindow = TrainModelWindow::create(gladeFile);

    if (TrainModelWindow)
    {
        TrainModelWindow->setModelPath(m_trainModelImagesPath, m_trainModelMasksPath);
        TrainModelWindow->present();
    }
}