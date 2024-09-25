#ifndef PREPROCESSWINDOW_H
#define PREPROCESSWINDOW_H

#include <gtkmm.h>
#include <iostream>
#include "fileutils.h"

class PreprocessWindow : public Gtk::Window
{
public:
    PreprocessWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static PreprocessWindow *create(const std::string &gladeFailePath);
    void setImagePreprocessingPath(const std::string &path);

protected:
    Gtk::ToggleButton *m_selectAreaToggleBtn;
    Gtk::Box *m_patchSettingsBox;
    Gtk::Label *m_imageNameLbl;
    Gtk::Label *m_imagePagingLbl;
    Gtk::Button *m_previousImageBtn;
    Gtk::Button *m_nextImageBtn;
    Gtk::DrawingArea *m_drawingArea;

    void on_window_shown();
    void onPreviousImageClicked();
    void onNextImageClicked();
    void onSelectedAreaToggled();
    bool onDrawingAreaDraw(const Cairo::RefPtr<Cairo::Context> &cr);

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_preprocessImagePath;
    std::vector<std::string> m_preprocessImageQueue;
    size_t m_preprocessImageIndex = 0;

};

#endif