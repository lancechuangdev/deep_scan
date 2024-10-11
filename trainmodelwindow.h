#ifndef TRAINMODELWINDOW_H
#define TRAINMODELWINDOW_H

#include <gtkmm.h>
#include <thread>
#include <fstream> // for std::ofstream
#include <cstdio> // for std::remove
#include "fileutils.h"
#include "pyscript.h"

class TrainModelWindow : public Gtk::Window
{
public:
    TrainModelWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &refGlade);
    static TrainModelWindow *create(const std::string &gladeFailePath);
    void setModelPath(const std::string &imagesPath, const std::string &masksPath);

protected:
    Gtk::Label *m_imagesCountLbl;
    Gtk::Label *m_masksCountLbl;
    Gtk::Label *m_imagesPathLbl;
    Gtk::Label *m_masksPathLbl;
    Gtk::Label *m_errorMsgLbl;
    Gtk::ComboBoxText *m_modelComboBox;
    Gtk::SpinButton *m_patchSizeSb;
    Gtk::SpinButton *m_batchSizeSb;
    Gtk::SpinButton *m_epochsSb;
    Gtk::SpinButton *m_predFidelitySb;
    Gtk::Entry *m_pyEnvEntry;
    Gtk::Button *m_startTrainingBtn;
    Gtk::LinkButton *m_viewModelBtn;
    Gtk::Button *m_testModelBtn;
    Gtk::LinkButton *m_viewTestResultBtn;
    void on_window_shown();
    void onStartTrainingClicked();
    void onViewModelClicked();
    void onTestModelClicked();
    void onViewTestResultClicked();

private:
    Glib::RefPtr<Gtk::Builder> m_refGlade;
    std::string m_imagesPath;
    std::string m_masksPath;
    std::string m_modelPath;
    std::string m_testImagesPath;
    std::string m_testMasksPath;
    std::string m_selectedModel;
    std::string m_pyEnv;
    int m_patchSize;
    int m_batchSize;
    int m_epochs;
    double m_predFidelity = 0.0;
    std::vector<std::string> m_selectedImages;
    std::vector<std::string> m_selectedMasks;
    static const std::string SettingsFilePath;
    bool validateInputDataset();
};

#endif
