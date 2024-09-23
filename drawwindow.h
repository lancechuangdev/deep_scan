#ifndef DRAWWINDOW_H
#define DRAWWINDOW_H

#include <iostream>
#include <filesystem>
#include <gtkmm.h>

class DrawWindow : public Gtk::Window
{
public:
    DrawWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static DrawWindow *create(const std::string &gladeFailePath);
    void setImageLabelingPath(const std::string &path);

protected:
    // Drawing image name, navigation and actions
    Gtk::ProgressBar *m_labelingPb;
    Gtk::Label *m_imageNameLbl;
    Gtk::Button *m_previousImageBtn;
    Gtk::Label *m_imagePagingLbl;
    Gtk::Button *m_nextImageBtn;
    Gtk::Switch *m_maskSwitch;
    Gtk::Button *m_saveMaskBtn;

    // Drawing toolbar
    Gtk::Button *m_selectImageBtn;
    Gtk::Button *m_roundBrushBtn;
    Gtk::Button *m_resetMaskBtn;

    // Drawing area
    Gtk::DrawingArea *m_drawingArea;

    // Window events
    void on_window_shown();
    bool onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr);

    // Button events
    void onPreviousImageClicked();
    void onNextImageClicked();
    void onSelectImageClicked();
    void onRoundBrushClicked();
    void onResetMaskClicked();
    void onSaveMaskClicked();

    // Mouse events
    bool onScrollEvent(GdkEventScroll *scroll_event);
    bool onButtonPressEvent(GdkEventButton *button_event);
    bool onButtonReleaseEvent(GdkEventButton *button_event);
    bool onMotionNotifyEvent(GdkEventMotion *motion_event);

    // Key events
    bool on_key_press_event(GdkEventKey *key_event) override;
    bool on_key_release_event(GdkEventKey *key_event) override;

    // Switch events
    void onMaskSwitchActiveChanged();

    // Drawing
    void InitializeMaskPixBuf(int width, int height);
    void clearDrawingArea();
    void loadDrawingAreaBuffer(bool showMask = false);
    void loadImageBufferFromFile(const std::string &filename); // Load the image buffer to the drawing area
    void LoadMaskBufferFromFile(const std::string &filename);
    void drawBrushCursor(const Cairo::RefPtr<Cairo::Context> &cr);
    void drawOnMask();
    void UpdateMaskAlpha(gint32 alpha);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_imageLabelingPath;
    std::vector<std::string> m_imageLabelingQueue;
    size_t m_imageLabelingIndex = 0;
    Glib::RefPtr<Gdk::Pixbuf> m_ImagePixbuf; // Store the currently loaded image pixbuf
    Glib::RefPtr<Gdk::Pixbuf> m_maskPixbuf;    // Mask layer (transparent surface)

    double m_zoomFactor = 1.0; // Zoom factor (1.0 = no zoom)
    double m_offsetX = 0.0;    // Horizontal pan offset
    double m_offsetY = 0.0;    // Vertical pan offset
    double m_dragStartX = 0.0; // Mouse drag start X
    double m_dragStartY = 0.0; // Mouse drag start Y

    bool m_isDrawingMode = false;   // Track whether it is drawing mode
    bool m_isDragging = false;      // Track whether the user is dragging
    bool m_isDrawing = false;       // Track whether the user is drawing
    bool m_showBrushCursor = false; // True when the brush cursor should be visible

    double m_brushRadius = 10.0; // Brush size
    double m_brushAlpha = 0.5;   // Opacity of the brush cursor
    double m_brushX;             // Brush cursor position (x)
    double m_brushY;             // Brush cursor position (y)

    bool m_ctrlPressed = false; // Flag to check if Ctrl key is pressed
    void saveMaskAsBinary(const std::string &filename);
};

#endif