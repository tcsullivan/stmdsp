#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"

#include <future>
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
    
    void onPaint(wxPaintEvent& pe);
    void onSinglePressed(wxCommandEvent& ce);
    void onConnectPressed(wxCommandEvent& ce);
    void onUploadPressed(wxCommandEvent& ce);
    void onRenderTimer(wxTimerEvent& te);

    void onFileNew(wxCommandEvent&);
    void onFileOpen(wxCommandEvent&);
    void onFileSave(wxCommandEvent&);
    void onFileSaveAs(wxCommandEvent&);

    void doSingle();
    void updateDrawing();

private:
    bool m_is_rendering = false;
    wxTimer *m_render_timer = nullptr;
    wxComboBox *m_device_combo = nullptr;
    wxStyledTextCtrl *m_text_editor = nullptr;
    wxControl *m_signal_area = nullptr;
    wxString m_open_file_path;

    stmdsp::device *m_device = nullptr;
    std::future<std::vector<stmdsp::adcsample_t>> m_device_samples_future;
    std::vector<stmdsp::adcsample_t> m_device_samples;

    bool tryDevice();
    void prepareEditor();
    wxString compileEditorCode();
};

#endif // WXMAIN_HPP_

