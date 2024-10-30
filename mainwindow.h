#ifndef DEEP_SCAN_MAINWINDOW_H
#define DEEP_SCAN_MAINWINDOW_H

#include <gtkmm.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <string>
#include <inttypes.h>  // For PRId64
#include <cstdint>     // For uintptr_t
#include "MvCameraControl.h"
#include "camcols.h"
#include "framequeue.h"
#include "drawwindow.h"
#include "preprocesswindow.h"
#include "trainmodelwindow.h"
#include "fileutils.h"
#include "logger.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger);
    virtual ~MainWindow();

protected:
    // Member widgets:
    Gtk::TreeView *m_camTreeView;
    Gtk::Button *m_discoverBtn;
    Gtk::Button *m_viewSettingsBtn;
    Gtk::Button *m_startCaptureBtn;
    Gtk::Button *m_stopCaptureBtn;
    Gtk::Button *m_openDrawingDialogBtn;
    Gtk::Button *m_openPatchDialogBtn;
    Gtk::Button *m_openTrainingDialogBtn;
    Gtk::Label *m_snLbl;
    Gtk::Entry *m_exposureTimeEntry;
    Gtk::Label *m_frameRateLbl;
    Gtk::Entry *m_widthEntry;
    Gtk::Entry *m_heightEntry;
    Gtk::Entry *m_offsetXEntry;
    Gtk::Entry *m_offsetYEntry;
    Gtk::Entry *m_gainEntry;

    Gtk::Button *m_saveSettingsBtn;
    Gtk::Button *m_recallSettingsBtn;
    Gtk::Button *m_uploadSettingsBtn;

    Gtk::ComboBoxText *m_cameraComboBox;
    Gtk::FileChooserButton *m_capturePickerFcb;
    Gtk::FileChooserButton *m_labelingPickerFcb;
    Gtk::FileChooserButton *m_patchPickerFcb;
    Gtk::FileChooserButton *m_modelImagesPickerFcb;
    Gtk::FileChooserButton *m_modelMasksPickerFcb;
    Gtk::SpinButton *m_captureRateSb;
    
    Gtk::FileChooserButton *m_modelPickerFcb;
    Gtk::FileChooserButton *m_testImagesPickerFcb;
    Gtk::FileChooserButton *m_testMasksPickerFcb;
    Gtk::Entry *m_pyEnvEntry;
    Gtk::SpinButton *m_confidenceThresholdSb;
    Gtk::SpinButton *m_patchSizeSb;
    Gtk::Button *m_testModelBtn;
    Gtk::LinkButton *m_viewTestResultBtn;

    // Signal handlers:
    void onDiscoverClicked();
    void onViewSettingsClicked();
    void onStartCaptureClicked();
    void onStopCaptureClicked();
    void onTreeviewSelectionChanged();
    void onOpenDrawingClicked();
    void onOpenPreprocessingClicked();
    void onOpenTrainingClicked();
    void onSavePresetClicked();
    void onRecallPresetClicked();
    void onUploadSettingsClicked();
    void onTestModelClicked();
    void onViewTestResultClicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    Glib::RefPtr<Gtk::ListStore> m_camListStore;
    MV_CC_DEVICE_INFO_LIST m_camList;
    CamColumns m_camcols;
    FrameQueue m_frameQueue;
    std::string m_imageLabelingPath;
    std::string m_preprocessImagePath;
    std::string m_trainModelImagesPath;
    std::string m_trainModelMasksPath;
    std::atomic<bool> m_isCapturing;
    std::vector<void*> m_deviceHandles;
    static const std::string SettingsFilePath;
    void populateDeviceSettings(void *deviceHandle);
    void clearDeviceSettings();
    void *getDeviceHandleBySerialNumber(std::string sn);
    std::vector<void*> getAllDeviceHandles();
    void preflight(void *deviceHandle);
    void startCapture(void *deviceHandle, double captureIntervalMs, std::string captureDestFolder);
    void stopCapture(void *deviceHandle);
    std::shared_ptr<Logger> m_logger;
};

#endif // DEEP_SCAN_MAINWINDOW_H
