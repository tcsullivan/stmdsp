#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"

#include <fstream>
#include <future>
#include <iostream>
#include <thread>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/timer.h>
#include <wx/wfstream.h>

class MainFrame : public wxFrame
{
public:
    MainFrame();

    void onCloseEvent(wxCloseEvent&);

    void onFileNew(wxCommandEvent&);
    void onFileOpen(wxCommandEvent&);
    void onFileOpenTemplate(wxCommandEvent&);
    void onFileSave(wxCommandEvent&);
    void onFileSaveAs(wxCommandEvent&);
    void onFileQuit(wxCommandEvent&);

    void onRunConnect(wxCommandEvent&);
    void onRunStart(wxCommandEvent&);
    void onRunUpload(wxCommandEvent&);
    void onRunUnload(wxCommandEvent&);
    void onRunEditBSize(wxCommandEvent&);
    void onRunEditSRate(wxCommandEvent&);
    void onRunGenUpload(wxCommandEvent&);
    void onRunGenStart(wxCommandEvent&);

    void onRunCompile(wxCommandEvent&);
    void onCodeDisassemble(wxCommandEvent&);

    void onMeasureTimer(wxTimerEvent& te);

private:
    bool m_is_running = false;
    wxComboBox *m_device_combo = nullptr;
    wxStyledTextCtrl *m_text_editor = nullptr;
    wxTextCtrl *m_compile_output = nullptr;
    wxControl *m_signal_area = nullptr;
    wxMenuItem *m_run_measure = nullptr;
    wxTimer *m_measure_timer = nullptr;
    wxStatusBar *m_status_bar = nullptr;
    wxMenuBar *m_menu_bar = nullptr;
    wxFileOutputStream *m_conv_result_log = nullptr;
    wxString m_open_file_path;
    wxString m_temp_file_name;

    stmdsp::device *m_device = nullptr;

    bool tryDevice();
    void prepareEditor();
    wxString compileEditorCode();
    wxMenu *loadTemplates();
};

#endif // WXMAIN_HPP_

