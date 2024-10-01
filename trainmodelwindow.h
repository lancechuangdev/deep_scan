#ifndef TRAINMODELWINDOW_H
#define TRAINMODELWINDOW_H

#include <gtkmm.h>
#include "fileutils.h"

class TrainModelWindow : public Gtk::Window
{
public:
    TrainModelWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static TrainModelWindow *create(const std::string &gladeFailePath);
    void setModelPath(const std::string &imagesPath, const std::string &masksPath, const std::string &modelPath);

protected:
    Gtk::Label *m_imagesCountLbl;
    Gtk::Label *m_masksCountLbl;
    Gtk::Label *m_imagesPathLbl;
    Gtk::Label *m_masksPathLbl;
    Gtk::Label *m_modelPathLbl;
    Gtk::Label *m_errorMsgLbl;
    Gtk::SpinButton *m_patchSizeSb;
    Gtk::Button *m_startTrainingBtn;
    void on_window_shown();
    void onStartTrainingClicked();

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_imagesPath;
    std::string m_masksPath;
    std::string m_modelPath;
    int m_patchSize;
    std::vector<std::string> m_selectedImages;
    std::vector<std::string> m_selectedMasks;
    bool validateDataset();
};

#endif