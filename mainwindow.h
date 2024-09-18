#ifndef DEEP_SCAN_MAINWINDOW_H
#define DEEP_SCAN_MAINWINDOW_H

#include <gtkmm/button.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>
#include <gtkmm/label.h>
#include <gtkmm/builder.h>
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
    Gtk::Window *m_drawWindow;
    Gtk::DrawingArea *m_drawingArea;

    // Signal handlers:
    void onDiscoverClicked();
    void onConnectClicked();
    void onStartClicked();
    void onStopClicked();
    void onDisconnectClicked();
    void onOpenDrawingClicked();
    void onTreeviewSelectionChanged();
    void onDrawWindowRealized();
    bool onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context>& cr);

    // Load and display an image in the drawing area
    void loadImage(const std::string& filename);

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
    std::vector<std::string> m_imageLabelingQueue;
    Glib::RefPtr<Gdk::Pixbuf> m_currentPixbuf; // Store the currently loaded pixbuf
    void populateDeviceSettings();
    void clearDeviceSettings();
};

#endif // DEEP_SCAN_MAINWINDOW_H
