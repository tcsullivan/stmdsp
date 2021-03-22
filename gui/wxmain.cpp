/**
 * @file wxmain.cpp
 * @brief Main window definition.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "wxmain.hpp"
#include "wxsiggen.hpp"

#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statusbr.h>
#include <wx/textdlg.h>

#include <array>
#include <sys/mman.h>
#include <vector>

#include "wxmain_devdata.h"

enum Id {
    MeasureTimer = 1,

    MFileNew,
    MFileOpen,
    MFileOpenTemplate,
    MFileSave,
    MFileSaveAs,
    MFileQuit,
    MRunConnect,
    MRunStart,
    MRunMeasure,
    MRunDrawSamples,
    MRunLogResults,
    MRunUpload,
    MRunUnload,
    MRunEditBSize,
    MRunGenUpload,
    MRunGenStart,
    MCodeCompile,
    MCodeDisassemble
};

#include "wxmain_mfile.h"
#include "wxmain_mrun.h"

MainFrame::MainFrame() :
    wxFrame(nullptr, wxID_ANY, "stmdspgui", wxDefaultPosition, wxSize(640, 800))
{
    // Main frame structure:
    // Begin with a main splitter for the code and terminal panes
    auto mainSplitter = new wxSplitterWindow(this, wxID_ANY);
    auto panelCode    = new wxPanel(mainSplitter, wxID_ANY);
    auto panelOutput  = new wxPanel(mainSplitter, wxID_ANY);
    // Additional panel for the toolbar
    auto panelToolbar = new wxPanel(this, wxID_ANY);
    // Sizers for the controls
    auto sizerToolbar = new wxBoxSizer(wxHORIZONTAL);
    auto sizerCode    = new wxBoxSizer(wxVERTICAL);
    auto sizerOutput  = new wxBoxSizer(wxVERTICAL);
    auto sizerMain    = new wxBoxSizer(wxVERTICAL);
    // Menu objects
    auto menuFile = new wxMenu;
    auto menuRun  = new wxMenu;
    auto menuCode = new wxMenu;

    // Member initialization
    m_status_bar     = new wxStatusBar(this);
    m_text_editor    = new wxStyledTextCtrl(panelCode, wxID_ANY,
                                            wxDefaultPosition, wxSize(620, 440));
    m_compile_output = new wxTextCtrl(panelOutput, wxID_ANY,
                                      wxEmptyString,
                                      wxDefaultPosition, wxSize(620, 250),
                                      wxTE_READONLY | wxTE_MULTILINE | wxHSCROLL);
    m_measure_timer  = new wxTimer(this, Id::MeasureTimer);
    m_menu_bar       = new wxMenuBar;
    m_rate_select    = new wxComboBox(panelToolbar, wxID_ANY,
                                      wxEmptyString,
                                      wxDefaultPosition, wxDefaultSize,
                                      srateValues.size(), srateValues.data(),
                                      wxCB_READONLY);
    m_device_samples = reinterpret_cast<stmdsp::adcsample_t *>(::mmap(
        nullptr, stmdsp::SAMPLES_MAX * sizeof(stmdsp::adcsample_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    m_device_samples_input = reinterpret_cast<stmdsp::adcsample_t *>(::mmap(
        nullptr, stmdsp::SAMPLES_MAX * sizeof(stmdsp::adcsample_t),
        PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));

    m_menu_bar->Append(menuFile, "&File");
    m_menu_bar->Append(menuRun, "&Run");
    m_menu_bar->Append(menuCode, "&Code");
    SetMenuBar(m_menu_bar);

    // Toolbar initialization
    auto comp = new wxButton(panelToolbar, Id::MCodeCompile, "Compile");
    sizerToolbar->Add(comp, 0, wxLEFT, 4);
    sizerToolbar->Add(m_rate_select, 0, wxLEFT, 12);
    panelToolbar->SetSizer(sizerToolbar);

    // Code panel init.
    prepareEditor();
    sizerCode->Add(panelToolbar, 0, wxTOP | wxBOTTOM, 4);
    sizerCode->Add(m_text_editor, 1, wxEXPAND, 0);
    panelCode->SetSizer(sizerCode);

    // Output panel init.
    m_compile_output->SetBackgroundColour(wxColour(0, 0, 0));
    m_compile_output->SetDefaultStyle(wxTextAttr(*wxWHITE, *wxBLACK, wxFont("Hack")));
    sizerOutput->Add(m_compile_output, 1, wxEXPAND | wxALL, 0);
    panelOutput->SetSizer(sizerOutput);

    // Main splitter init.
    mainSplitter->SetSashGravity(0.5);
    mainSplitter->SetMinimumPaneSize(20);
    mainSplitter->SplitHorizontally(panelCode, panelOutput, 440);
    sizerMain->Add(mainSplitter, 1, wxEXPAND, 5);
    sizerMain->SetSizeHints(this);
    SetSizer(sizerMain);

    m_status_bar->SetStatusText("Ready.");
    SetStatusBar(m_status_bar);

    // Binds:

    // General
    Bind(wxEVT_TIMER,        &MainFrame::onMeasureTimer, this, Id::MeasureTimer);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onCloseEvent,   this, wxID_ANY);
    Bind(wxEVT_PAINT,        &MainFrame::onPaint,        this, wxID_ANY);

    // Toolbar actions
    Bind(wxEVT_BUTTON,   &MainFrame::onRunCompile,        this, Id::MCodeCompile, wxID_ANY, comp);
    Bind(wxEVT_COMBOBOX, &MainFrame::onToolbarSampleRate, this, wxID_ANY,         wxID_ANY, m_rate_select);


    // File menu actions
    Bind(wxEVT_MENU, &MainFrame::onFileNew,    this, Id::MFileNew,    wxID_ANY, menuFile->Append(MFileNew, "&New"));
    Bind(wxEVT_MENU, &MainFrame::onFileOpen,   this, Id::MFileOpen,   wxID_ANY, menuFile->Append(MFileOpen, "&Open"));
    menuFile->Append(MFileOpenTemplate, "Open &Template", loadTemplates());
    Bind(wxEVT_MENU, &MainFrame::onFileSave,   this, Id::MFileSave,   wxID_ANY, menuFile->Append(MFileSave, "&Save"));
    Bind(wxEVT_MENU, &MainFrame::onFileSaveAs, this, Id::MFileSaveAs, wxID_ANY, menuFile->Append(MFileSaveAs, "Save &As"));
    menuFile->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onFileQuit,   this, Id::MFileQuit,   wxID_ANY, menuFile->Append(MFileQuit, "&Quit"));

    // Run menu actions
    Bind(wxEVT_MENU, &MainFrame::onRunConnect,    this, Id::MRunConnect,    wxID_ANY, menuRun->Append(MRunConnect, "&Connect"));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunStart,      this, Id::MRunStart,      wxID_ANY, menuRun->Append(MRunStart, "&Start"));
    m_run_measure = menuRun->AppendCheckItem(MRunMeasure, "&Measure code time");
    m_run_draw_samples = menuRun->AppendCheckItem(MRunDrawSamples, "&Draw samples");
    Bind(wxEVT_MENU, &MainFrame::onRunLogResults, this, Id::MRunLogResults, wxID_ANY, menuRun->AppendCheckItem(MRunLogResults, "&Log results..."));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunUpload,     this, Id::MRunUpload,     wxID_ANY, menuRun->Append(MRunUpload, "&Upload code"));
    Bind(wxEVT_MENU, &MainFrame::onRunUnload,     this, Id::MRunUnload,     wxID_ANY, menuRun->Append(MRunUnload, "U&nload code"));
    Bind(wxEVT_MENU, &MainFrame::onRunEditBSize,  this, Id::MRunEditBSize,  wxID_ANY, menuRun->Append(MRunEditBSize, "Set &buffer size..."));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunGenUpload,  this, Id::MRunGenUpload,  wxID_ANY, menuRun->Append(MRunGenUpload, "&Load signal generator..."));
    Bind(wxEVT_MENU, &MainFrame::onRunGenStart,   this, Id::MRunGenStart,   wxID_ANY, menuRun->AppendCheckItem(MRunGenStart, "Start &generator"));

    // Code menu actions
    Bind(wxEVT_MENU, &MainFrame::onRunCompile,      this, Id::MCodeCompile,     wxID_ANY, menuCode->Append(MCodeCompile, "&Compile code"));
    Bind(wxEVT_MENU, &MainFrame::onCodeDisassemble, this, Id::MCodeDisassemble, wxID_ANY, menuCode->Append(MCodeDisassemble, "Show &Disassembly"));
    menuCode->AppendSeparator();

    updateMenuOptions();
}

// Closes the window
// Needs to clean things up
void MainFrame::onCloseEvent(wxCloseEvent& event)
{
    SetMenuBar(nullptr);
    //delete m_menu_bar->Remove(2);
    //delete m_menu_bar->Remove(1);
    //delete m_menu_bar->Remove(0);
    //delete m_menu_bar;
    delete m_measure_timer;
    delete m_device;

    Unbind(wxEVT_COMBOBOX, &MainFrame::onToolbarSampleRate, this, wxID_ANY,         wxID_ANY);
    Unbind(wxEVT_BUTTON,   &MainFrame::onRunCompile,        this, Id::MCodeCompile, wxID_ANY);

    event.Skip();
}

// Measure timer tick handler
// Only called while connected and running.
void MainFrame::onMeasureTimer(wxTimerEvent&)
{
    if (m_conv_result_log || m_run_draw_samples->IsChecked()) {
        auto samples = m_device->continuous_read();
        if (samples.size() > 0) {
            std::copy(samples.cbegin(), samples.cend(), m_device_samples);

            if (m_conv_result_log) {
                for (auto& s : samples) {
                    auto str = std::to_string(s);
                    m_conv_result_log->Write(str.c_str(), str.size());
                }
            }
            if (m_run_draw_samples->IsChecked()) {
                samples = m_device->continuous_read_input();
                std::copy(samples.cbegin(), samples.cend(), m_device_samples_input);
                this->Refresh();
            }
        }
    }

    if (m_wav_clip) {
        // Stream out next WAV chunk
        auto size = m_device->get_buffer_size();
        auto chunk = new stmdsp::adcsample_t[size];
        auto src = reinterpret_cast<uint16_t *>(m_wav_clip->next(size));
        for (unsigned int i = 0; i < size; i++)
            chunk[i] = ((uint32_t)*src++) / 16 + 2048;
        m_device->siggen_upload(chunk, size);
        delete[] chunk;
    }

    if (m_run_measure->IsChecked()) {
        // Show execution time
        m_status_bar->SetStatusText(wxString::Format(wxT("Execution time: %u cycles"),
                                                     m_device->continuous_start_get_measurement()));
    }
}

void MainFrame::onPaint(wxPaintEvent&)
{
    if (!m_is_running || !m_run_draw_samples->IsChecked()) {
        if (!m_compile_output->IsShown())
            m_compile_output->Show();
        return;
    } else if (m_compile_output->IsShown()) {
        m_compile_output->Hide();
    }

    auto py = m_compile_output->GetScreenPosition().y - this->GetScreenPosition().y - 28;
    wxRect rect {
        0, py,
        this->GetSize().GetWidth(),
        this->GetSize().GetHeight() - py - 60
    };

    auto *dc = new wxPaintDC(this);
    dc->SetBrush(*wxBLACK_BRUSH);
    dc->SetPen(*wxBLACK_PEN);
    dc->DrawRectangle(rect);
    auto stoy = [&](stmdsp::adcsample_t s) {
        return static_cast<float>(py) + rect.GetHeight() -
            (static_cast<float>(rect.GetHeight()) * s / 4095.f);
    };
    auto scount = m_device->get_buffer_size();
    float dx = static_cast<float>(rect.GetWidth()) / scount;
    float x = 0;
    float lasty = stoy(2048);
    dc->SetBrush(wxBrush(wxColour(0xFF, 0, 0, 0x80)));
    dc->SetPen(wxPen(wxColour(0xFF, 0, 0, 0x80)));
    for (decltype(scount) i = 0; i < scount; i++) {
        auto y = stoy(m_device_samples[i]);
        dc->DrawLine(x, lasty, x + dx, y);
        x += dx, lasty = y;
    }
    x = 0;
    lasty = stoy(2048);
    dc->SetBrush(wxBrush(wxColour(0, 0, 0xFF, 0x80)));
    dc->SetPen(wxPen(wxColour(0, 0, 0xFF, 0x80)));
    for (decltype(scount) i = 0; i < scount; i++) {
        auto y = stoy(m_device_samples_input[i]);
        dc->DrawLine(x, lasty, x + dx, y);
        x += dx, lasty = y;
    }
    delete dc;
}

void MainFrame::prepareEditor()
{
    m_text_editor->SetLexer(wxSTC_LEX_CPP);
    m_text_editor->SetMarginWidth(0, 30);
    m_text_editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    m_text_editor->StyleSetFaceName(wxSTC_STYLE_DEFAULT, "Hack");
    m_text_editor->StyleClearAll();
    m_text_editor->SetTabWidth(4);
    m_text_editor->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColor(75, 75, 75));
    m_text_editor->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColor(220, 220, 220));

    m_text_editor->StyleSetForeground(wxSTC_C_STRING,            wxColour(150,0,0));
    m_text_editor->StyleSetForeground(wxSTC_C_PREPROCESSOR,      wxColour(165,105,0));
    m_text_editor->StyleSetForeground(wxSTC_C_IDENTIFIER,        wxColour(40,0,60));
    m_text_editor->StyleSetForeground(wxSTC_C_NUMBER,            wxColour(0,150,0));
    m_text_editor->StyleSetForeground(wxSTC_C_CHARACTER,         wxColour(150,0,0));
    m_text_editor->StyleSetForeground(wxSTC_C_WORD,              wxColour(0,0,150));
    m_text_editor->StyleSetForeground(wxSTC_C_WORD2,             wxColour(0,150,0));
    m_text_editor->StyleSetForeground(wxSTC_C_COMMENT,           wxColour(150,150,150));
    m_text_editor->StyleSetForeground(wxSTC_C_COMMENTLINE,       wxColour(150,150,150));
    m_text_editor->StyleSetForeground(wxSTC_C_COMMENTDOC,        wxColour(150,150,150));
    m_text_editor->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, wxColour(0,0,200));
    m_text_editor->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, wxColour(0,0,200));
    m_text_editor->StyleSetBold(wxSTC_C_WORD, true);
    m_text_editor->StyleSetBold(wxSTC_C_WORD2, true);
    m_text_editor->StyleSetBold(wxSTC_C_COMMENTDOCKEYWORD, true);

    // a sample list of keywords, I haven't included them all to keep it short...
    m_text_editor->SetKeyWords(0,
        wxT("return for while do break continue if else goto asm"));
    m_text_editor->SetKeyWords(1,
        wxT("void char short int long auto float double unsigned signed "
            "volatile static const constexpr constinit consteval "
            "virtual final noexcept public private protected"));
    wxCommandEvent dummy;
    onFileNew(dummy);
}

wxString MainFrame::compileEditorCode()
{
    if (m_device == nullptr) {
        m_status_bar->SetStatusText("Need device connected to compile.");
        return "";
    }

    if (m_temp_file_name.IsEmpty())
        m_temp_file_name = wxFileName::CreateTempFileName("stmdspgui");

    wxFile file (m_temp_file_name, wxFile::write);
    wxString file_text (m_device->get_platform() == stmdsp::platform::L4 ? file_header_l4
                                                                         : file_header_h7);
    file_text.Replace("$0", std::to_string(m_device ? m_device->get_buffer_size()
                                                    : stmdsp::SAMPLES_MAX));
    file.Write(wxString(file_text) + m_text_editor->GetText());
    file.Close();

    wxFile makefile (m_temp_file_name + "make", wxFile::write);
    wxString make_text (m_device->get_platform() == stmdsp::platform::L4 ? makefile_text_l4
                                                                         : makefile_text_h7);
    make_text.Replace("$0", m_temp_file_name);
    makefile.Write(make_text);
    makefile.Close();

    wxString make_output = m_temp_file_name + "make.log";
    if (wxFile::Exists(make_output))
        wxRemoveFile(make_output);

    wxString make_command = wxString("make -C ") + m_temp_file_name.BeforeLast('/') +
                            " -f " + m_temp_file_name + "make" +
                            " > " + make_output + " 2>&1";

    int result = system(make_command.ToAscii());
    m_compile_output->LoadFile(make_output);

    if (result == 0) {
        m_status_bar->SetStatusText("Compilation succeeded.");
        return m_temp_file_name + ".o";
    } else {
        m_status_bar->SetStatusText("Compilation failed.");
        return "";
    }
}

void MainFrame::onToolbarSampleRate(wxCommandEvent& ce)
{
    auto combo = dynamic_cast<wxComboBox *>(ce.GetEventUserData());
    m_device->set_sample_rate(combo->GetCurrentSelection());
    m_status_bar->SetStatusText("Ready.");
}

void MainFrame::onCodeDisassemble(wxCommandEvent&)
{
    if (!m_temp_file_name.IsEmpty()) {
        auto output = m_temp_file_name + ".asm.log";
        wxString command = wxString("arm-none-eabi-objdump -d --no-show-raw-insn ") +
                                    m_temp_file_name + ".orig.o" // +
                                    " > " + output + " 2>&1";
    
        if (system(command.ToAscii()) == 0) {
            m_compile_output->LoadFile(output);
            m_status_bar->SetStatusText(wxString::Format(wxT("Done. Line count: %u."),
                                                             m_compile_output->GetNumberOfLines()));
        } else {
            m_compile_output->ChangeValue("");
            m_status_bar->SetStatusText("Failed to load disassembly.");
        }
    } else {
        m_compile_output->ChangeValue("");
        m_status_bar->SetStatusText("Need to compile code before analyzing.");
    }
}

wxMenu *MainFrame::loadTemplates()
{
    wxMenu *menu = new wxMenu;

    wxArrayString files;
    if (wxDir::GetAllFiles(wxGetCwd() + "/templates", &files, "*.cpp", wxDIR_FILES) > 0) {
        files.Sort();
        int id = 1000;
        for (auto file : files) {
            Bind(wxEVT_MENU, &MainFrame::onFileOpenTemplate, this, id, wxID_ANY,
                menu->Append(id, file.AfterLast('/')));
            id++;
        }
    }

    return menu;
}

void MainFrame::updateMenuOptions()
{
    bool connected = m_device != nullptr;
    m_menu_bar->Enable(MRunStart, connected);
    m_menu_bar->Enable(MRunUpload, connected);
    m_menu_bar->Enable(MRunUnload, connected);
    m_menu_bar->Enable(MRunEditBSize, connected);
    m_menu_bar->Enable(MRunGenUpload, connected);
    m_menu_bar->Enable(MRunGenStart, connected);
    m_rate_select->Enable(connected);
}

