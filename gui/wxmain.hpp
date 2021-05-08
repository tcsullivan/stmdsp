/**
 * @file wxmain.hpp
 * @brief Main window definition.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"
#include "wav.hpp"

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
    void onRunLogResults(wxCommandEvent&);
    void onRunUpload(wxCommandEvent&);
    void onRunUnload(wxCommandEvent&);
    void onRunEditBSize(wxCommandEvent&);
    void onRunGenUpload(wxCommandEvent&);
    void onRunGenStart(wxCommandEvent&);

    void onToolbarSampleRate(wxCommandEvent&);

    void onRunCompile(wxCommandEvent&);
    void onCodeDisassemble(wxCommandEvent&);

    void onPaint(wxPaintEvent&);
    void onTimerPerformance(wxTimerEvent&);
    void onTimerRecord(wxTimerEvent&);
    void onTimerWavClip(wxTimerEvent&);

private:
    // Set to true if connected and running
    bool m_is_running = false;

    wxComboBox *m_device_combo = nullptr;
    wxStyledTextCtrl *m_text_editor = nullptr;
    wxTextCtrl *m_compile_output = nullptr;
    wxControl *m_signal_area = nullptr;
    wxMenuItem *m_run_measure = nullptr;
    wxMenuItem *m_run_draw_samples = nullptr;
    wxTimer *m_timer_performance = nullptr;
    wxTimer *m_timer_record = nullptr;
    wxTimer *m_timer_wavclip = nullptr;
    wxStatusBar *m_status_bar = nullptr;
    wxMenuBar *m_menu_bar = nullptr;
    wxComboBox *m_rate_select = nullptr;

    // File handle for logging output samples
    // Not null when logging is enabled
    wxFileOutputStream *m_conv_result_log = nullptr;
    // File path of currently opened file
    // Empty if new file
    wxString m_open_file_path;
    // File path for temporary files (e.g. compiled ELF)
    // Set by compile action
    wxString m_temp_file_name;

    // Device interface
    // Not null if connected
    stmdsp::device *m_device = nullptr;
    stmdsp::adcsample_t *m_device_samples = nullptr;
    stmdsp::adcsample_t *m_device_samples_input = nullptr;
    // WAV data for signal generator
    // Not null when a WAV is loaded
    wav::clip *m_wav_clip = nullptr;

    bool tryDevice();
    void prepareEditor();
    wxString compileEditorCode();
    wxMenu *loadTemplates();
    // Updates control availabilities based on device connection
    void updateMenuOptions();
};

#endif // WXMAIN_HPP_

