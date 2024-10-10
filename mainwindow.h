#ifndef DEEP_SCAN_MAINWINDOW_H
#define DEEP_SCAN_MAINWINDOW_H

#include <gtkmm.h>
#include "MvCameraControl.h"
#include "camcols.h"
#include "framequeue.h"
#include "drawwindow.h"
#include "preprocesswindow.h"
#include "trainmodelwindow.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <filesystem>
#include <cstdio>
#include <fstream>
#include <inttypes.h>  // For PRId64
#include <cstdint>     // For uintptr_t

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder);
    virtual ~MainWindow();

protected:
    // Member widgets:
    Gtk::TreeView *m_camTreeView;
    Gtk::Button *m_discoverBtn;
    Gtk::Button *m_viewSettingsBtn;
    Gtk::Button *m_startBtn;
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
    Gtk::FileChooserButton *m_saveModelPickerFcb;
    Gtk::SpinButton *m_captureDurationSb;
    Gtk::SpinButton *m_captureRateSb;
    Gtk::ProgressBar *m_capturePb;
    
    // Signal handlers:
    void onDiscoverClicked();
    void onViewSettingsClicked();
    void onStartClicked();
    void onTreeviewSelectionChanged();
    void onOpenDrawingClicked();
    void onOpenPreprocessingClicked();
    void onOpenTrainingClicked();
    void onSavePresetClicked();
    void onRecallPresetClicked();
    void onUploadSettingsClicked();

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
    void populateDeviceSettings(void *deviceHandle);
    void clearDeviceSettings();
    void *getDeviceHandleBySerialNumber(std::string sn);
    std::vector<void*> getAllDeviceHandles();
    void captureImages(void *deviceHandle, int captureDurationSec, double captureIntervalMs, std::string captureDestFolder);
    static const std::string SettingsFilePath;
};

#endif // DEEP_SCAN_MAINWINDOW_H
