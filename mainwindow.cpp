#include "mainwindow.h"

const std::string MainWindow::SettingsFilePath = std::string(std::getenv("HOME")) + "/.config/deep-scan/settings.ini";

MainWindow::MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger)
    : Gtk::Window(obj),
      m_builder(refBuilder),
      m_frameQueue(20),
      m_logger(logger)
{
    // Set the window title
    Gtk::Window *root;
    m_builder->get_widget("root", root);
    root->set_title("Deep Scan");

    // Get the button by ID and connect the signal handler.
    m_builder->get_widget("discover_btn", m_discoverBtn);
    m_builder->get_widget("view_settings_btn", m_viewSettingsBtn);
    m_builder->get_widget("start_capture_btn", m_startCaptureBtn);
    m_builder->get_widget("stop_capture_btn", m_stopCaptureBtn);
    m_builder->get_widget("open_drawing_btn", m_openDrawingDialogBtn);
    m_builder->get_widget("open_patch_btn", m_openPatchDialogBtn);
    m_builder->get_widget("open_training_btn", m_openTrainingDialogBtn);

    // Disable buttons initially
    m_viewSettingsBtn -> set_sensitive(false);
    m_startCaptureBtn->set_sensitive(false);
    m_openDrawingDialogBtn->set_sensitive(false);
    m_openPatchDialogBtn->set_sensitive(false);
    m_openTrainingDialogBtn->set_sensitive(false);

    if (m_discoverBtn)
    {
        m_discoverBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onDiscoverClicked));
    }
    if (m_viewSettingsBtn)
    {
        m_viewSettingsBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onViewSettingsClicked));
    }
    if (m_startCaptureBtn)
    {
        m_startCaptureBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStartCaptureClicked));
    }
    if (m_stopCaptureBtn)
    {
        m_stopCaptureBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onStopCaptureClicked));
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
    m_camTreeView->append_column("Serial Number", m_camcols.col_sn);

    // Device settings    
    m_builder->get_widget("sn_lbl", m_snLbl);
    m_builder->get_widget("exposure_entry", m_exposureTimeEntry);
    m_builder->get_widget("frame_rate_lbl", m_frameRateLbl);
    m_builder->get_widget("width_entry", m_widthEntry);
    m_builder->get_widget("height_entry", m_heightEntry);
    m_builder->get_widget("offset_x_entry", m_offsetXEntry);
    m_builder->get_widget("offset_y_entry", m_offsetYEntry);
    m_builder->get_widget("gain_entry", m_gainEntry);

    m_builder->get_widget("save_settings_btn", m_saveSettingsBtn);
    if (m_saveSettingsBtn)
    {
        m_saveSettingsBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onSavePresetClicked));
    }
    m_builder->get_widget("recall_settings_btn", m_recallSettingsBtn);
    if (m_recallSettingsBtn)
    {
        m_recallSettingsBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onRecallPresetClicked));
    }
    m_builder->get_widget("upload_settings_btn", m_uploadSettingsBtn);
    if (m_uploadSettingsBtn)
    {
        m_uploadSettingsBtn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::onUploadSettingsClicked));
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

void MainWindow::onTreeviewSelectionChanged()
{
    int nIndex = getSelectedCamIndex(m_camTreeView, m_camListStore);
    m_viewSettingsBtn->set_sensitive(nIndex >= 0);
}

void MainWindow::onDiscoverClicked()
{
    // Clear the TreeView before adding new data
    m_camListStore->clear();
    m_cameraComboBox->remove_all();

    do
    {
        memset(&m_camList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

        // enum device
        int nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &m_camList);
        if (nRet != MV_OK)
        {
            std::cout << "MV_CC_EnumDevices fail! Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_EnumDevices: " + std::to_string(nRet), Logger::ERROR);
            break;
        }

        if (m_camList.nDeviceNum > 0)
        {
            for (unsigned int i = 0; i < m_camList.nDeviceNum; i++)
            {
                MV_CC_DEVICE_INFO *pDeviceInfo = m_camList.pDeviceInfo[i];
                if (NULL == pDeviceInfo)
                {
                    break;
                }

                Gtk::TreeModel::Row row = *(m_camListStore->append());
                if (pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
                {
                    auto modelName = pDeviceInfo->SpecialInfo.stGigEInfo.chModelName;
                    auto friendlyName = pDeviceInfo->SpecialInfo.stGigEInfo.chUserDefinedName;
                    auto serialNumber = pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
                    row[m_camcols.col_model] = Glib::ustring(reinterpret_cast<const char *>(modelName));
                    row[m_camcols.col_friendly_name] = Glib::ustring(reinterpret_cast<const char *>(friendlyName));
                    row[m_camcols.col_ip] = getIpV4AddressString(pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp);
                    row[m_camcols.col_sn] = Glib::ustring(reinterpret_cast<const char *>(serialNumber));

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
            break;
        }
    } while (false);
}

void MainWindow::onViewSettingsClicked()
{
    int nIndex = getSelectedCamIndex(m_camTreeView, m_camListStore);
    if (nIndex < 0)
    {
        std::cout << "No camera was selected." << std::endl;
        m_logger->log("No camera was selected.");
        return;
    }

    void *deviceHandle;
    MV_CC_DEVICE_INFO *pSelectedCam = m_camList.pDeviceInfo[nIndex];
    int nRet = MV_CC_CreateHandle(&deviceHandle, pSelectedCam);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_CreateHandle fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CreateHandle: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Connect device
    nRet = MV_CC_OpenDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    populateDeviceSettings(deviceHandle);

    // Close device
    nRet = MV_CC_CloseDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
    }

    // Destroy handle
    nRet = MV_CC_DestroyHandle(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::populateDeviceSettings(void *deviceHandle)
{
    // Serial number
    MVCC_STRINGVALUE sn = {0};
    int nRet = MV_CC_GetStringValue(deviceHandle, "DeviceSerialNumber", &sn);
    if (MV_OK == nRet && m_snLbl)
    {
        m_snLbl->set_text(Glib::ustring(sn.chCurValue));
    }

    // Exposure time
    MVCC_FLOATVALUE exposureTime = {0};
    nRet = MV_CC_GetFloatValue(deviceHandle, "ExposureTime", &exposureTime);
    if (MV_OK == nRet && m_exposureTimeEntry)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << exposureTime.fCurValue;

        // Set the label text
        m_exposureTimeEntry->set_text(Glib::ustring(oss.str()));
    }
    else
    {
        std::cout << "Failed to get exposure time. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
    }

    // Resulting Frame Rate
    MVCC_FLOATVALUE frameRate = {0};
    nRet = MV_CC_GetFloatValue(deviceHandle, "ResultingFrameRate", &frameRate);
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
        m_logger->log("Error on MV_CC_GetFloatValue(ResultingFrameRate): " + std::to_string(nRet), Logger::ERROR);
    }

    // Width
    MVCC_INTVALUE width = {0};
    nRet = MV_CC_GetIntValue(deviceHandle, "Width", &width);
    if (MV_OK == nRet && m_widthEntry)
    {
        m_widthEntry->set_text(std::to_string(width.nCurValue));
    }
    else
    {
        std::cout << "Failed to get width. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
    }

    // Height
    MVCC_INTVALUE height = {0};
    nRet = MV_CC_GetIntValue(deviceHandle, "Height", &height);
    if (MV_OK == nRet && m_heightEntry)
    {
        m_heightEntry->set_text(std::to_string(height.nCurValue));
    }
    else
    {
        std::cout << "Failed to get height. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
    }

    // Offset X
    MVCC_INTVALUE offsetX = {0};
    nRet = MV_CC_GetIntValue(deviceHandle, "OffsetX", &offsetX);
    if (MV_OK == nRet && m_offsetXEntry)
    {
        m_offsetXEntry->set_text(std::to_string(offsetX.nCurValue));
    }
    else
    {
        std::cout << "Failed to get offsetX. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
    }

    // Offset Y
    MVCC_INTVALUE offsetY = {0};
    nRet = MV_CC_GetIntValue(deviceHandle, "OffsetY", &offsetY);
    if (MV_OK == nRet && m_offsetYEntry)
    {
        m_offsetYEntry->set_text(std::to_string(offsetY.nCurValue));
    }
    else
    {
        std::cout << "Failed to get offsetY. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
    }

    // Gain
    MVCC_FLOATVALUE gain = {0};
    nRet = MV_CC_GetFloatValue(deviceHandle, "Gain", &gain);
    if (MV_OK == nRet && m_gainEntry)
    {
        // Convert float to string
        std::ostringstream oss;
        oss << gain.fCurValue;

        // Set the label text
        m_gainEntry->set_text(oss.str());
    }
    else
    {
        std::cout << "Failed to get gain. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_GetFloatValue(Gain): " + std::to_string(nRet), Logger::ERROR);
    }
}

void MainWindow::clearDeviceSettings()
{
    if (m_snLbl)
    {
        m_snLbl->set_text(std::string());
    }
    if (m_exposureTimeEntry)
    {
        m_exposureTimeEntry->set_text(std::string());
    }
    if (m_frameRateLbl)
    {
        m_frameRateLbl->set_text(std::string());
    }
    if (m_widthEntry)
    {
        m_widthEntry->set_text(std::string());
    }
    if (m_heightEntry)
    {
        m_heightEntry->set_text(std::string());
    }
    if (m_offsetXEntry)
    {
        m_offsetXEntry->set_text(std::string());
    }
    if (m_offsetYEntry)
    {
        m_offsetYEntry->set_text(std::string());
    }
    if (m_gainEntry)
    {
        m_gainEntry->set_text(std::string());
    }
}

void MainWindow::onSavePresetClicked()
{
    if (!FileUtils::createFile(SettingsFilePath))
    {
        return;
    }

    // Step 1: Read the existing content of the file
    std::ifstream settingsFile(SettingsFilePath);
    std::stringstream buffer;
    if (settingsFile.is_open())
    {
        buffer << settingsFile.rdbuf();
        settingsFile.close();
    }
    else
    {
        std::cerr << "Unable to open settings file: " << SettingsFilePath << std::endl;
        m_logger->log("Unable to open settings file: " + SettingsFilePath, Logger::ERROR);
    }

    std::string content = buffer.str();
    std::string serialNumber;
    if (m_snLbl)
    {
        serialNumber = m_snLbl->get_text();
    }
    std::string sectionHeader = "[" + serialNumber + "]";
    std::stringstream sectionContent;
    sectionContent << sectionHeader << std::endl;
    if (m_exposureTimeEntry)
    {
        sectionContent << "exposureTime=" << m_exposureTimeEntry->get_text() << std::endl;
    }
    if (m_widthEntry)
    {
        sectionContent << "width=" << m_widthEntry->get_text() << std::endl;
    }
    if (m_heightEntry)
    {
        sectionContent << "height=" << m_heightEntry->get_text() << std::endl;
    }
    if (m_offsetXEntry)
    {
        sectionContent << "offsetX=" << m_offsetXEntry->get_text() << std::endl;
    }
    if (m_offsetYEntry)
    {
        sectionContent << "offsetY=" << m_offsetYEntry->get_text() << std::endl;
    }
    if (m_gainEntry)
    {
        sectionContent << "gain=" << m_gainEntry->get_text() << std::endl;
    }
    sectionContent << std::endl; // Add a blank line after the new section

    // Step 2: Find if the section for the device already exists
    size_t sectionPos = content.find(sectionHeader);
    bool sectionExists = (sectionPos != std::string::npos);

    if (sectionExists)
    {
        // Step 3: If the section exists, replace its contents
        size_t nextSectionPos = content.find('[', sectionPos + 1); // Find the next section's starting position

        // Replace the old section with the new one
        if (nextSectionPos == std::string::npos)
        {
            // The section is the last one, so replace to the end of the file
            content.replace(sectionPos, std::string::npos, sectionContent.str());
        }
        else
        {
            // Replace up to the next section
            content.replace(sectionPos, nextSectionPos - sectionPos, sectionContent.str());
        }
    }
    else
    {
        // Step 4: If the section doesn't exist, append the new section at the end
        content += sectionContent.str();
    }

    // Step 5: Write the updated content back to the file (overwrite)
    std::ofstream outFile(SettingsFilePath);
    if (outFile.is_open()) 
    {
        outFile << content;
        outFile.close();
    }
    else
    {
        std::cerr << "Unable to open settings file for writing." << std::endl;
        m_logger->log("Unable to open settings file for writing: " + SettingsFilePath, Logger::ERROR);
    }
}

void MainWindow::onRecallPresetClicked()
{
    std::ifstream settingsFile(SettingsFilePath);
    std::string line;
    bool isCurrentDevice = false;
    std::string sn;

    if (m_snLbl)
    {
        sn = m_snLbl->get_text();
    }

    if (settingsFile.is_open())
    {
        while (std::getline(settingsFile, line))
        {
            if (line == "[" + sn + "]")
            {
                isCurrentDevice = true;
            }
            else if (line.find('[') != std::string::npos)
            {
                isCurrentDevice = false; // New section means we passed the current device's settings
            }

            if (isCurrentDevice)
            {
                std::istringstream lineStream(line);
                std::string key;

                if (std::getline(lineStream, key, '='))
                {
                    std::string value;
                    if (key == "exposureTime" && std::getline(lineStream, value))
                    {
                        if (m_exposureTimeEntry)
                        {
                            m_exposureTimeEntry->set_text(Glib::ustring(value));
                        }
                    }
                    else if (key == "width" && std::getline(lineStream, value))
                    {
                        if (m_widthEntry)
                        {
                            m_widthEntry->set_text(Glib::ustring(value));
                        }
                    }
                    else if (key == "height" && std::getline(lineStream, value))
                    {
                        if (m_heightEntry)
                        {
                            m_heightEntry->set_text(Glib::ustring(value));
                        }
                    }
                    else if (key == "offsetX" && std::getline(lineStream, value))
                    {
                        if (m_offsetXEntry)
                        {
                            m_offsetXEntry->set_text(Glib::ustring(value));
                        }
                    }
                    else if (key == "offsetY" && std::getline(lineStream, value))
                    {
                        if (m_offsetYEntry)
                        {
                            m_offsetYEntry->set_text(Glib::ustring(value));
                        }
                    }
                    else if (key == "gain" && std::getline(lineStream, value))
                    {
                        if (m_gainEntry)
                        {
                            m_gainEntry->set_text(Glib::ustring(value));
                        }
                    }
                }
            }
        }
        settingsFile.close();
    }
}

void MainWindow::onUploadSettingsClicked()
{
    // Create device handle
    void* deviceHandle = nullptr;
    if (!m_snLbl)
    {
        return;
    }
    deviceHandle = getDeviceHandleBySerialNumber(m_snLbl->get_text());
    if (deviceHandle == nullptr)
    {
        std::cout << "getDeviceHandleBySerialNumber fail! deviceHandle is nullptr" << std::endl;
        m_logger->log("Error on getDeviceHandleBySerialNumber, deviceHandle is nullptr.", Logger::ERROR);
        return;
    }

    // Connect to the device
    int nRet = MV_CC_OpenDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_OpenDevice fail! Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_OpenDevice: " + std::to_string(nRet), Logger::ERROR);
        return;
    }

    // Set device settings
    if (m_exposureTimeEntry)
    {
        auto exposureTime = m_exposureTimeEntry->get_text();
        nRet = MV_CC_SetFloatValue(deviceHandle, "ExposureTime", std::stof(exposureTime));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set exposure time. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetFloatValue(ExposureTime): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    if (m_widthEntry)
    {
        auto width = m_widthEntry->get_text();
        nRet = MV_CC_SetIntValue(deviceHandle, "Width", std::stoi(width));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set width. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(Width): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    if (m_heightEntry)
    {
        auto height = m_heightEntry->get_text();
        nRet = MV_CC_SetIntValue(deviceHandle, "Height", std::stoi(height));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set height. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(Height): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    if (m_offsetXEntry)
    {
        auto offsetX = m_offsetXEntry->get_text();
        nRet = MV_CC_SetIntValue(deviceHandle, "OffsetX", std::stoi(offsetX));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set offsetX. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(OffsetX): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    if (m_offsetYEntry)
    {
        auto offsetY = m_offsetYEntry->get_text();
        nRet = MV_CC_SetIntValue(deviceHandle, "OffsetY", std::stoi(offsetY));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set offsetY. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetIntValue(OffsetY): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    if (m_gainEntry)
    {
        auto gain = m_gainEntry->get_text();
        nRet = MV_CC_SetFloatValue(deviceHandle, "Gain", std::stof(gain));
        if (MV_OK != nRet)
        {
            std::cerr << "Error to set gain. Error code: " << nRet << std::endl;
            m_logger->log("Error on MV_CC_SetFloatValue(Gain): " + std::to_string(nRet), Logger::ERROR);
        }
    }

    // Close device
    nRet = MV_CC_CloseDevice(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_CloseDevice fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_CloseDevice: " + std::to_string(nRet), Logger::ERROR);
    }

    // Destroy handle
    nRet = MV_CC_DestroyHandle(deviceHandle);
    if (nRet != MV_OK)
    {
        std::cout << "MV_CC_DestroyHandle fail. Error code: " << nRet << std::endl;
        m_logger->log("Error on MV_CC_DestroyHandle: " + std::to_string(nRet), Logger::ERROR);
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
    TrainModelWindow *TrainModelWindow = TrainModelWindow::create(gladeFile, m_logger);

    if (TrainModelWindow)
    {
        TrainModelWindow->setModelPath(m_trainModelImagesPath, m_trainModelMasksPath);
        TrainModelWindow->present();
    }
}