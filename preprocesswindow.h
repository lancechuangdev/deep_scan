#ifndef PREPROCESSWINDOW_H
#define PREPROCESSWINDOW_H

#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include "fileutils.h"

class PreprocessWindow : public Gtk::Window
{
public:
    PreprocessWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static PreprocessWindow *create(const std::string &gladeFailePath);
    void setImagePreprocessingPath(const std::string &path);

protected:
    Gtk::RadioButton *m_selectRadioBtn;
    Gtk::RadioButton *m_selectROIRadioBtn;
    Gtk::Box *m_patchSettingsBox;
    Gtk::Label *m_imageNameLbl;
    Gtk::Label *m_imagePagingLbl;
    Gtk::Button *m_previousImageBtn;
    Gtk::Button *m_nextImageBtn;
    Gtk::DrawingArea *m_drawingArea;
    Gtk::ListBox *m_thumbnailsListbox;
    Gtk::SpinButton *m_patchSizeSb;
    Gtk::SpinButton *m_patchWidthSb;
    Gtk::SpinButton *m_patchHeightSb;

    void on_window_shown();
    void onPreviousImageClicked();
    void onNextImageClicked();
    void onSelectToggled();
    void onPatchToggled();
    bool onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr);
    void onDeleteRow(Gtk::Box* row_box, const std::string& image, const std::string& mask);
    void onViewPatch(Gtk::Box *row_box, const std::string &image);

    // Mouse events
    bool onScrollEvent(GdkEventScroll *scroll_event);
    bool onButtonPressEvent(GdkEventButton *button_event);
    bool onButtonReleaseEvent(GdkEventButton *button_event);
    bool onMotionNotifyEvent(GdkEventMotion *motion_event);

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    void loadPatchThumbnails();
    void loadDrawingAreaBuffer();
    void loadImageBufferFromFile(const std::string &filename);
    void LoadMaskBufferFromFile(const std::string &filename);
    void UpdateMaskAlpha(gint32 alpha);
    void drawBrushCursor(const Cairo::RefPtr<Cairo::Context> &cr);
    void drawOnSelectedROI();
    bool loadMetadata(const std::string &metadataFilename, int &patchSize, double &top, double &left);
    bool saveMetadata(const std::string &metadataFilename, int patchSize, double top, double left);
    bool saveImagePatch(const std::string &filename, double top, double left);
    void saveMaskAsBinary(const std::string &filename, double top, double left);
    void addThumbnailsToList(const std::string& image, const std::string& mask);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_preprocessImagePath;
    std::vector<std::string> m_preprocessImageQueue;
    size_t m_preprocessImageIndex = 0;
    Glib::RefPtr<Gdk::Pixbuf> m_ImagePixbuf; // Store the currently loaded image pixbuf
    Glib::RefPtr<Gdk::Pixbuf> m_maskPixbuf;    // Mask layer (transparent surface)
    Glib::RefPtr<Gdk::Pixbuf> m_patchPixbuf;    // Patch layer

    double m_zoomFactor = 1.0; // Zoom factor (1.0 = no zoom)
    double m_offsetX = 0.0;    // Horizontal pan offset
    double m_offsetY = 0.0;    // Vertical pan offset
    double m_dragStartX = 0.0; // Mouse drag start X
    double m_dragStartY = 0.0; // Mouse drag start Y
    double m_brushAlpha = 0.5;   // Opacity of the brush cursor
    double m_brushX;             // Brush cursor position (x)
    double m_brushY;             // Brush cursor position (y)

    int m_patchSize = 512;
    int m_patchWidth = 1;
    int m_patchHeight = 1;

    bool m_isDragging = false;      // Track whether the user is dragging
    bool m_ctrlPressed = false; // Flag to check if Ctrl key is pressed
    bool m_showPatchCursor = false; // True when the brush cursor should be visible
    bool m_isPatchingMode = false;
};

#endif