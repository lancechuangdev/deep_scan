#ifndef DEEP_SCAN_MAINWINDOW_H
#define DEEP_SCAN_MAINWINDOW_H

#include <gtkmm.h>
#include "MvCameraControl.h"
#include "camcols.h"
#include "framequeue.h"
#include "drawwindow.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <filesystem>

class MainWindow : public Gtk::Window
{
public:
    MainWindow(BaseObjectType *obj, Glib::RefPtr<Gtk::Builder> const &refBuilder);
    virtual ~MainWindow();

protected:
    // Member widgets:
    Gtk::TreeView *m_camTreeView;
    Gtk::Button *m_discoverBtn;
    Gtk::Button *m_connectBtn;
    Gtk::Button *m_startBtn;
    Gtk::Button *m_stopBtn;
    Gtk::Button *m_disconnectBtn;
    Gtk::Button *m_openDrawingDialogBtn;
    Gtk::Label *m_exposureTimeLbl;
    Gtk::Label *m_frameRateLbl;
    Gtk::Label *m_widthLbl;
    Gtk::Label *m_heightLbl;
    Gtk::Label *m_gainLbl;
    Gtk::FileChooserButton *m_capturePickerFcb;
    Gtk::FileChooserButton *m_loadPickerFcb;
    Gtk::SpinButton *m_captureDurationSb;
    Gtk::SpinButton *m_captureRateSb;
    Gtk::ProgressBar *m_capturePb;

    // Signal handlers:
    void onDiscoverClicked();
    void onConnectClicked();
    void onStartClicked();
    void onStopClicked();
    void onDisconnectClicked();
    void onOpenDrawingClicked();
    void onTreeviewSelectionChanged();

private:
    Glib::RefPtr<Gtk::Builder> m_builder;
    Glib::RefPtr<Gtk::ListStore> m_camListStore;
    MV_CC_DEVICE_INFO_LIST m_camList;
    CamColumns m_camcols;
    void *m_selectedCam;
    std::string m_imageFolderPath;
    int64_t m_lastCaptureTimestamp;
    int m_captureDuration;
    double m_captureInterval;
    sigc::connection m_captureTimeoutConnection;
    int m_captureElapsedTime; // in milliseconds
    FrameQueue m_frameQueue;
    std::atomic<bool> m_running;
    std::string m_imageLabelingPath;
    void populateDeviceSettings();
    void clearDeviceSettings();
};

#endif // DEEP_SCAN_MAINWINDOW_H
