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

static const std::array<wxString, 6> srateValues {
    "8 kS/s",
    "16 kS/s",
    "20 kS/s",
    "32 kS/s",
    "48 kS/s",
    "96 kS/s"
};
static const std::array<unsigned int, 6> srateNums {
    8000,
    16000,
    20000,
    32000,
    48000,
    96000
};

static const char *makefile_text_h7 = R"make(
all:
	@arm-none-eabi-g++ -x c++ -Os -fno-exceptions -fno-rtti \
                       -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 -mtune=cortex-m7 \
	                   -nostartfiles \
	                   -Wl,-Ttext-segment=0x00000000 -Wl,-zmax-page-size=512 -Wl,-eprocess_data_entry \
	                   $0 -o $0.o
	@cp $0.o $0.orig.o
	@arm-none-eabi-strip -s -S --strip-unneeded $0.o
	@arm-none-eabi-objcopy --remove-section .ARM.attributes \
                           --remove-section .comment \
                           --remove-section .noinit \
                           $0.o
	arm-none-eabi-size $0.o
)make";
static const char *makefile_text_l4 = R"make(
all:
	@arm-none-eabi-g++ -x c++ -Os -fno-exceptions -fno-rtti \
	                   -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mtune=cortex-m4 \
	                   -nostartfiles \
	                   -Wl,-Ttext-segment=0x10000000 -Wl,-zmax-page-size=512 -Wl,-eprocess_data_entry \
	                   $0 -o $0.o
	@cp $0.o $0.orig.o
	@arm-none-eabi-strip -s -S --strip-unneeded $0.o
	@arm-none-eabi-objcopy --remove-section .ARM.attributes \
                           --remove-section .comment \
                           --remove-section .noinit \
                           $0.o
	arm-none-eabi-size $0.o
)make";

static const char *file_header_h7 = R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;
constexpr unsigned int SIZE = $0;
adcsample_t *process_data(adcsample_t *samples, unsigned int size);
extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

constexpr double PI = 3.14159265358979323846L;
__attribute__((naked))
auto sin(double x) {
asm("vmov.f64 r1, r2, d0;"
    "eor r0, r0;"
    "svc 1;"
    "vmov.f64 d0, r1, r2;"
    "bx lr");
return 0;
}
__attribute__((naked))
auto cos(double x) {
asm("vmov.f64 r1, r2, d0;"
	"mov r0, #1;"
	"svc 1;"
	"vmov.f64 d0, r1, r2;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto tan(double x) {
asm("vmov.f64 r1, r2, d0;"
	"mov r0, #2;"
	"svc 1;"
	"vmov.f64 d0, r1, r2;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto sqrt(double x) {
asm("vsqrt.f64 d0, d0; bx lr");
return 0;
}

// End stmdspgui header code

)cpp";
static const char *file_header_l4 = R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;
constexpr unsigned int SIZE = $0;
adcsample_t *process_data(adcsample_t *samples, unsigned int size);
extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

constexpr float PI = 3.14159265358979L;
__attribute__((naked))
auto sin(float x) {
asm("vmov.f32 r1, s0;"
    "eor r0, r0;"
    "svc 1;"
    "vmov.f32 s0, r1;"
    "bx lr");
return 0;
}
__attribute__((naked))
auto cos(float x) {
asm("vmov.f32 r1, s0;"
	"mov r0, #1;"
	"svc 1;"
	"vmov.f32 s0, r1;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto tan(double x) {
asm("vmov.f32 r1, s0;"
	"mov r0, #2;"
	"svc 1;"
	"vmov.f32 s0, r1;"
	"bx lr");
return 0;
}
__attribute__((naked))
auto sqrt(float) {
asm("vsqrt.f32 s0, s0; bx lr");
return 0;
}

// End stmdspgui header code

)cpp";


static const char *file_content = 
R"cpp(adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
    return samples;
}
)cpp";


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

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "stmdspgui", wxPoint(50, 50), wxSize(640, 800))
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
    dc->SetBrush(*wxRED_BRUSH);
    dc->SetPen(*wxRED_PEN);
    auto stoy = [&](stmdsp::adcsample_t s) {
        return static_cast<float>(py) + rect.GetHeight() -
            (static_cast<float>(rect.GetHeight()) * s / 4095.f);
    };
    auto scount = m_device->get_buffer_size();
    float dx = static_cast<float>(rect.GetWidth()) / scount;
    float x = 0;
    float lasty = stoy(2048);
    for (decltype(scount) i = 0; i < scount; i++) {
        auto y = stoy(m_device_samples[i]);
        dc->DrawLine(x, lasty, x + dx, y);
        x += dx, lasty = y;
    }
    dc->SetBrush(*wxBLUE_BRUSH);
    dc->SetPen(*wxBLUE_PEN);
    x = 0;
    lasty = stoy(2048);
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

void MainFrame::onFileNew(wxCommandEvent&)
{
    m_open_file_path = "";
    m_text_editor->SetText(file_content);
    m_text_editor->DiscardEdits();
    m_status_bar->SetStatusText("Ready.");
}

void MainFrame::onFileOpen(wxCommandEvent&)
{
    wxFileDialog openDialog(this, "Open filter file", "", "",
                            "C++ source file (*.cpp)|*.cpp",
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openDialog.ShowModal() != wxID_CANCEL) {
        if (wxFileInputStream file_stream (openDialog.GetPath()); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new char[size + 1];
            buffer[size] = '\0';
            if (file_stream.ReadAll(buffer, size)) {
                m_open_file_path = openDialog.GetPath();
                m_text_editor->SetText(buffer);
                m_text_editor->DiscardEdits();
                m_compile_output->ChangeValue("");
                m_status_bar->SetStatusText("Ready.");
            } else {
                m_status_bar->SetStatusText("Failed to read file contents.");
            }
            delete[] buffer;
        } else {
            m_status_bar->SetStatusText("Failed to open file.");
        }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onFileOpenTemplate(wxCommandEvent& event)
{
    auto file_path = wxGetCwd() + "/templates/" + m_menu_bar->GetLabel(event.GetId());

    if (wxFileInputStream file_stream (file_path); file_stream.IsOk()) {
        auto size = file_stream.GetSize();
        auto buffer = new char[size + 1];
        buffer[size] = '\0';
        if (file_stream.ReadAll(buffer, size)) {
            m_open_file_path = "";
            m_text_editor->SetText(buffer);
            //m_text_editor->DiscardEdits();
            m_status_bar->SetStatusText("Ready.");
        } else {
            m_status_bar->SetStatusText("Failed to read file contents.");
        }
        delete[] buffer;
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}


void MainFrame::onFileSave(wxCommandEvent& ce)
{
    if (m_text_editor->IsModified()) {
        if (m_open_file_path.IsEmpty()) {
            onFileSaveAs(ce);
        } else {
            if (wxFile file (m_open_file_path, wxFile::write); file.IsOpened()) {
                file.Write(m_text_editor->GetText());
                file.Close();
                m_text_editor->DiscardEdits();
                m_status_bar->SetStatusText("Saved.");
            } else {
                m_status_bar->SetStatusText("Save failed: couldn't open file.");
            }
        }
    } else {
        m_status_bar->SetStatusText("No modifications to save.");
    }
}

void MainFrame::onFileSaveAs(wxCommandEvent&)
{
    if (m_text_editor->IsModified()) {
        wxFileDialog saveDialog(this, "Save filter file", "", "",
                                "C++ source file (*.cpp)|*.cpp",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveDialog.ShowModal() != wxID_CANCEL) {
            if (wxFile file (saveDialog.GetPath(), wxFile::write); file.IsOpened()) {
                file.Write(m_text_editor->GetText());
                file.Close();
                m_text_editor->DiscardEdits();
                m_open_file_path = saveDialog.GetPath();
                m_status_bar->SetStatusText("Saved.");
            } else {
                m_status_bar->SetStatusText("Save failed: couldn't open file.");
            }
        }
    } else {
        m_status_bar->SetStatusText("No modifications to save.");
    }
}

void MainFrame::onFileQuit(wxCommandEvent&)
{
    Close(true);
}

void MainFrame::onRunConnect(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_device) {
        stmdsp::scanner scanner;
        if (auto devices = scanner.scan(); devices.size() > 0) {
            m_device = new stmdsp::device(devices.front());
            if (m_device->connected()) {
                auto rate = m_device->get_sample_rate();
                m_rate_select->SetSelection(rate);

                updateMenuOptions();
                menuItem->SetItemLabel("&Disconnect");
                m_status_bar->SetStatusText("Connected.");
            } else {
                delete m_device;
                m_device = nullptr;

                menuItem->SetItemLabel("&Connect");
                m_status_bar->SetStatusText("Failed to connect.");
            }
        } else {
            m_status_bar->SetStatusText("No devices found.");
        }
    } else {
        delete m_device;
        m_device = nullptr;
        updateMenuOptions();
        menuItem->SetItemLabel("&Connect");
        m_status_bar->SetStatusText("Disconnected.");
    }
}

void MainFrame::onRunStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_is_running) {
        if (m_run_measure->IsChecked()) {
            m_device->continuous_start_measure();
            m_measure_timer->StartOnce(1000);
        } else {
            if (m_device->is_siggening() && m_wav_clip) {
                m_measure_timer->Start(m_device->get_buffer_size() * 500 / 
                                       srateNums[m_rate_select->GetSelection()]);
            } else if (m_conv_result_log) {
                m_measure_timer->Start(15);
            } else if (m_run_draw_samples->IsChecked()) {
                m_measure_timer->Start(300);
            }

            m_device->continuous_start();
        }

        m_rate_select->Enable(false);
        menuItem->SetItemLabel("&Stop");
        m_status_bar->SetStatusText("Running.");
        m_is_running = true;
    } else {
        m_device->continuous_stop();
        m_measure_timer->Stop();

        m_rate_select->Enable(true);
        menuItem->SetItemLabel("&Start");
        m_status_bar->SetStatusText("Ready.");
        m_is_running = false;

        if (m_run_draw_samples->IsChecked())
            m_compile_output->Refresh();
    }
}

void MainFrame::onRunLogResults(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (menuItem->IsChecked()) {
        wxFileDialog dialog (this, "Choose log file", "", "", "*.csv",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_CANCEL) {
            if (m_conv_result_log) {
                m_conv_result_log->Close();
                delete m_conv_result_log;
                m_conv_result_log = nullptr;
            }

            m_conv_result_log = new wxFileOutputStream(dialog.GetPath());
        }

        m_status_bar->SetStatusText("Ready.");
    } else if (m_conv_result_log) {
        m_conv_result_log->Close();
        delete m_conv_result_log;
        m_conv_result_log = nullptr;
    }
}

void MainFrame::onRunEditBSize(wxCommandEvent&)
{
    wxTextEntryDialog dialog (this, "Enter new buffer size (100-4096)", "Set Buffer Size");
    if (dialog.ShowModal() == wxID_OK) {
        if (wxString value = dialog.GetValue(); !value.IsEmpty()) {
            if (unsigned long n; value.ToULong(&n)) {
                if (n >= 100 && n <= stmdsp::SAMPLES_MAX) {
                    m_device->continuous_set_buffer_size(n);
                } else {
                    m_status_bar->SetStatusText("Error: Invalid buffer size.");
                }
            } else {
                m_status_bar->SetStatusText("Error: Invalid buffer size.");
            }
        } else {
            m_status_bar->SetStatusText("Ready.");
        }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onToolbarSampleRate(wxCommandEvent& ce)
{
    auto combo = dynamic_cast<wxComboBox *>(ce.GetEventUserData());
    m_device->set_sample_rate(combo->GetCurrentSelection());
    m_status_bar->SetStatusText("Ready.");
}

void MainFrame::onRunGenUpload(wxCommandEvent&)
{
    wxTextEntryDialog dialog (this, "Enter up to 8000 generator values below. "
                                    "Values must be whole numbers between zero and 4095.",
                                    "Enter Generator Values");
    if (dialog.ShowModal() == wxID_OK) {
        if (wxString values = dialog.GetValue(); !values.IsEmpty()) {
            if (values[0] == '/') {
                m_wav_clip = new wav::clip(values.Mid(1));
                if (m_wav_clip->valid()) {
                    m_status_bar->SetStatusText("Generator ready.");
                } else {
                    delete m_wav_clip;
                    m_wav_clip = nullptr;
                    m_status_bar->SetStatusText("Error: Bad WAV file.");
                }
            } else {
                std::vector<stmdsp::dacsample_t> samples;
                while (!values.IsEmpty() && samples.size() <= stmdsp::SAMPLES_MAX * 2) {
                    if (auto number_end = values.find_first_not_of("0123456789");
                        number_end != wxString::npos && number_end > 0)
                    {
                        auto number = values.Left(number_end);
                        if (unsigned long n; number.ToULong(&n))
                            samples.push_back(n & 4095);

                        if (auto next = values.find_first_of("0123456789", number_end + 1);
                            next != wxString::npos)
                        {
                            values = values.Mid(next);
                        } else {
                            break;
                        }
                    } else {
                        break;
                    }
                }

                if (samples.size() <= stmdsp::SAMPLES_MAX) {
                    m_device->siggen_upload(&samples[0], samples.size());
                    m_status_bar->SetStatusText("Generator ready.");
                } else {
                    m_status_bar->SetStatusText("Error: Too many samples (max is 8000).");
                }
            }
        } else {
            m_status_bar->SetStatusText("Error: No samples given.");
        }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onRunGenStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (menuItem->IsChecked()) {
        m_device->siggen_start();
        menuItem->SetItemLabel("Stop &generator");
        m_status_bar->SetStatusText("Generator running.");
    } else {
        m_device->siggen_stop();
        menuItem->SetItemLabel("Start &generator");
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onRunUpload(wxCommandEvent&)
{
    if (auto file = compileEditorCode(); !file.IsEmpty()) {
        if (wxFileInputStream file_stream (file); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new unsigned char[size];
            file_stream.ReadAll(buffer, size);
            m_device->upload_filter(buffer, size);
            m_status_bar->SetStatusText("Code uploaded.");
        } else {
             m_status_bar->SetStatusText("Couldn't load compiled code.");
        }
    }
}

void MainFrame::onRunUnload(wxCommandEvent&)
{
    m_device->unload_filter();
    m_status_bar->SetStatusText("Unloaded code.");
}

void MainFrame::onRunCompile(wxCommandEvent&)
{
    compileEditorCode();
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

