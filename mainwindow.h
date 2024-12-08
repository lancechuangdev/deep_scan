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
#include "fileutils.h"
#include "logger.h"

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder, std::shared_ptr<Logger> logger);
    virtual ~MainWindow();

protected:
    Gtk::Stack *m_main_stack;
    Gtk::RadioButton *m_menu_capture_rbtn;
    Gtk::RadioButton *m_menu_annotation_rbtn;
    Gtk::RadioButton *m_menu_training_rbtn;
    Gtk::RadioButton *m_menu_test_rbtn;
    Gtk::Button *m_discoverBtn;
    Gtk::Button *m_startCaptureBtn;
    Gtk::Button *m_stopCaptureBtn;
    Gtk::Button *m_annotation_filter_btn;
    Gtk::Button *m_openDrawingDialogBtn;
    Gtk::Button *m_openPatchDialogBtn;
    Gtk::Label *m_snLbl;
    Gtk::Label *m_frameRateLbl;
    Gtk::Entry *m_annotation_py_env_entry;
    Gtk::SpinButton *m_annotation_pred_fidelity_sb;

    Gtk::ComboBoxText *m_cameraComboBox;
    Gtk::FileChooserButton *m_capturePickerFcb;
    Gtk::FileChooserButton *m_filter_picker_fcb;
    Gtk::FileChooserButton *m_labelingPickerFcb;
    Gtk::FileChooserButton *m_patchPickerFcb;
    Gtk::FileChooserButton *m_modelImagesPickerFcb;
    Gtk::FileChooserButton *m_modelMasksPickerFcb;
    Gtk::SpinButton *m_captureRateSb;
    
    Gtk::FileChooserButton *m_augmentation_picker_fcb;
    Gtk::Entry *m_augmentation_number_entry;
    Gtk::SpinButton *m_augmentation_patch_size_sb;
    Gtk::Entry *m_augmentation_py_env_entry;
    Gtk::Button *m_augment_btn;

    Gtk::ComboBoxText *m_modelComboBox;
    Gtk::SpinButton *m_patchSizeSb;
    Gtk::SpinButton *m_batchSizeSb;
    Gtk::Entry *m_pyEnvEntry;
    Gtk::SpinButton *m_epochsSb;
    Gtk::Button *m_startTrainingBtn;
    Gtk::LinkButton *m_viewModelBtn;
    Gtk::Label *m_test_images_dir_lbl;
    Gtk::Label *m_test_masks_dir_lbl;
    Gtk::SpinButton *m_predFidelitySb;
    Gtk::Button *m_testModelBtn;
    Gtk::LinkButton *m_viewTestResultBtn;

    // Signal handlers:
    void on_window_shown();
    void onDiscoverClicked();
    void onStartCaptureClicked();
    void onStopCaptureClicked();
    void onFilterImagesClicked();
    void onOpenDrawingClicked();
    void onOpenPreprocessingClicked();
    void onAugmentClicked();
    void onStartTrainingClicked();
    void onViewModelClicked();
    void onTestModelClicked();
    void onViewTestResultClicked();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    Glib::RefPtr<Gtk::ListStore> m_camListStore;
    MV_CC_DEVICE_INFO_LIST m_camList;
    FrameQueue m_frameQueue;
    std::string m_imageLabelingPath;
    std::string m_preprocessImagePath;
    std::atomic<bool> m_isCapturing;
    std::vector<void*> m_deviceHandles;
    static const std::string SettingsFilePath;
    void on_menu_toggled();
    void *getDeviceHandleBySerialNumber(std::string sn);
    std::vector<void*> getAllDeviceHandles();
    void preflight(void *deviceHandle);
    void startCapture(void *deviceHandle, double captureIntervalMs, std::string captureDestFolder);
    void stopCapture(void *deviceHandle);
    std::shared_ptr<Logger> m_logger;
};

#endif // DEEP_SCAN_MAINWINDOW_H
