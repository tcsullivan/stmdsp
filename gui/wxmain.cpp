#include "wxmain.hpp"

#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>

enum Id {
    Single = 1,
    ConnectDevice,
    UploadFilter,
    RenderTimer,

    MFileNew,
    MFileOpen,
    MFileSave,
    MFileSaveAs,
    MFileQuit,
    MRunConnect,
    MRunStart,
    MRunMeasure,
    MRunCompile,
    MRunUpload,
    MRunUnload
};

MainFrame::MainFrame() : wxFrame(nullptr, -1, "stmdspgui", wxPoint(50, 50), wxSize(640, 800))
{
    auto menubar = new wxMenuBar;
    auto menuFile = new wxMenu;
    Bind(wxEVT_MENU, &MainFrame::onFileNew, this, Id::MFileNew, wxID_ANY,
         menuFile->Append(MFileNew, "&New"));
    Bind(wxEVT_MENU, &MainFrame::onFileOpen, this, Id::MFileOpen, wxID_ANY, menuFile->Append(MFileOpen, "&Open"));
    Bind(wxEVT_MENU, &MainFrame::onFileSave, this, Id::MFileSave, wxID_ANY,
         menuFile->Append(MFileSave, "&Save"));
    Bind(wxEVT_MENU, &MainFrame::onFileSaveAs, this, Id::MFileSaveAs, wxID_ANY,
         menuFile->Append(MFileSaveAs, "Save &As"));
    menuFile->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onFileQuit, this, Id::MFileQuit, wxID_ANY,
         menuFile->Append(MFileQuit, "&Quit"));

    auto menuRun = new wxMenu;
    Bind(wxEVT_MENU, &MainFrame::onRunConnect, this, Id::MRunConnect, wxID_ANY,
         menuRun->Append(MRunConnect, "&Connect"));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunStart, this, Id::MRunStart, wxID_ANY,
         menuRun->Append(MRunStart, "&Start"));
    m_run_measure = menuRun->AppendCheckItem(MRunMeasure, "&Measure code speed");
    m_run_measure_value = menuRun->Append(wxID_ANY, "Last time:");
    m_run_measure_value->Enable(false);
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunCompile, this, Id::MRunCompile, wxID_ANY,
         menuRun->Append(MRunCompile, "&Compile code"));
    Bind(wxEVT_MENU, &MainFrame::onRunUpload, this, Id::MRunUpload, wxID_ANY,
         menuRun->Append(MRunUpload, "&Upload code"));
    Bind(wxEVT_MENU, &MainFrame::onRunUnload, this, Id::MRunUnload, wxID_ANY,
         menuRun->Append(MRunUnload, "U&nload code"));

    menubar->Append(menuFile, "&File");
    menubar->Append(menuRun, "&Run");
    SetMenuBar(menubar);

    auto window = new wxBoxSizer(wxVERTICAL);

    m_text_editor = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(620, 700));
    prepareEditor();
    window->Add(m_text_editor, 1, wxEXPAND | wxALL, 10);

    //m_signal_area = new wxControl(this, wxID_ANY, wxDefaultPosition, wxSize(600, 200));
    //window->Add(m_signal_area, 1, wxEXPAND | wxALL, 10);

    SetSizerAndFit(window);

    //m_render_timer = new wxTimer(this, Id::RenderTimer);

    //Bind(wxEVT_PAINT, &MainFrame::onPaint, this, wxID_ANY);
    //Bind(wxEVT_TIMER, &MainFrame::onRenderTimer, this, Id::RenderTimer);
}

void MainFrame::onPaint([[maybe_unused]] wxPaintEvent& pe)
{
    auto *dc = new wxClientDC(this);
    auto region = m_signal_area->GetRect();
    dc->SetClippingRegion(region);

    dc->SetBrush(*wxBLACK_BRUSH); dc->SetPen(*wxBLACK_PEN);
    dc->DrawRectangle(region);

    if (m_device_samples.size() > 0) {
        dc->SetPen(*wxRED_PEN);
        auto points = new wxPoint[m_device_samples.size()];
        const float spacing = static_cast<float>(region.GetWidth()) / m_device_samples.size();
        float x = 0;
        for (auto ptr = points; auto sample : m_device_samples) {
            *ptr++ = wxPoint {
                static_cast<int>(x),
                region.GetHeight() - sample * region.GetHeight() / 4096
            };
            x += spacing;
        }
        dc->DrawLines(m_device_samples.size(), points, region.GetX(), region.GetY());
        delete[] points;
    }
}

void MainFrame::onRenderTimer([[maybe_unused]] wxTimerEvent& te)
{
    updateDrawing();
}

void MainFrame::updateDrawing()
{
    if (m_device_samples = m_device_samples_future.get(); m_device_samples.size() > 0)
        this->RefreshRect(m_signal_area->GetRect());

    requestSamples();
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
        wxT("return for while do break continue if else goto"));
    m_text_editor->SetKeyWords(1,
        wxT("void char short int long auto float double unsigned signed "
            "volatile static const constexpr constinit consteval "
            "virtual final noexcept public private protected"));
    wxCommandEvent dummy;
    onFileNew(dummy);
}

static const char *makefile_text = R"make(
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
)make";

static wxString file_header (R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;

adcsample_t *process_data(adcsample_t *samples, unsigned int size);

extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

// End stmdspgui header code

)cpp");

wxString MainFrame::compileEditorCode()
{
    static wxString temp_file_name;

    if (temp_file_name.IsEmpty())
        temp_file_name = wxFileName::CreateTempFileName("stmdspgui");

    wxFile file (temp_file_name, wxFile::write);
    file.Write(file_header + m_text_editor->GetText());
    file.Close();

    wxFile makefile (temp_file_name + "make", wxFile::write);
    wxString make_text (makefile_text);
    make_text.Replace("$0", temp_file_name);
    makefile.Write(make_text);
    makefile.Close();

    wxString make_command = wxString("make -C ") + temp_file_name.BeforeLast('/') +
                            " -f " + temp_file_name + "make";
    if (system(make_command.ToAscii()) == 0)
        return temp_file_name + ".o";
    else
        return "";
}

void MainFrame::onFileNew([[maybe_unused]] wxCommandEvent&)
{
    m_open_file_path = "";
    m_text_editor->SetText(
R"cpp(adcsample_t *process_data(adcsample_t *samples, unsigned int size)
{
    return samples;
}
)cpp");
    m_text_editor->DiscardEdits();
}

void MainFrame::onFileOpen([[maybe_unused]] wxCommandEvent&)
{
    wxFileDialog openDialog(this, "Open filter file", "", "",
                            "C++ source file (*.cpp)|*.cpp",
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openDialog.ShowModal() != wxID_CANCEL) {
        if (wxFileInputStream file_stream (openDialog.GetPath()); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new char[size];
            if (file_stream.ReadAll(buffer, size)) {
                m_open_file_path = openDialog.GetPath();
                m_text_editor->SetText(buffer);
                m_text_editor->DiscardEdits();
            }
            delete[] buffer;
        }
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
            }
        }
    }
}

void MainFrame::onFileSaveAs([[maybe_unused]] wxCommandEvent& ce)
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
            }
        }
    }
}

void MainFrame::onFileQuit([[maybe_unused]] wxCommandEvent&)
{
    Close(true);
}

void MainFrame::onRunConnect(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (m_device == nullptr) {
        stmdsp::scanner scanner;
        if (auto devices = scanner.scan(); devices.size() > 0) {
            m_device = new stmdsp::device(devices.front());
            if (m_device->connected()) {
                menuItem->SetItemLabel("&Disconnect");
            } else {
                delete m_device;
                m_device = nullptr;
                menuItem->SetItemLabel("&Connect");
            }
        }
    } else {
        delete m_device;
        m_device = nullptr;
        menuItem->SetItemLabel("&Connect");
    }
}

void MainFrame::onRunStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_render_timer->IsRunning()) {
        if (m_device != nullptr && m_device->connected()) {
            if (m_run_measure && m_run_measure->IsChecked())
                m_device->continuous_start_measure();
            else
                m_device->continuous_start();

            m_device_samples_future = std::async(std::launch::async,
                                                 []() { return decltype(m_device_samples)(); });
            m_render_timer->Start(1000);
            menuItem->SetItemLabel("&Stop");
        } else {
            wxMessageBox("No device connected!", "Run", wxICON_WARNING);
        }
    } else {
        m_render_timer->Stop();
        m_device->continuous_stop();

        menuItem->SetItemLabel("&Start");
        if (m_run_measure && m_run_measure_value) {
            m_run_measure_value->SetItemLabel(
                m_run_measure->IsChecked() ? wxString::Format(wxT("Last time: %u"),m_device->continuous_start_get_measurement())
                                           : "Last time: --");
        }
    }
}

void MainFrame::onRunCompile([[maybe_unused]] wxCommandEvent&)
{
    compileEditorCode();
}

void MainFrame::onRunUpload([[maybe_unused]] wxCommandEvent&)
{
    if (auto file = compileEditorCode(); !file.IsEmpty()) {
        if (wxFileInputStream file_stream (file); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new unsigned char[size];
            if (m_device != nullptr && m_device->connected()) {
                file_stream.ReadAll(buffer, size);
                m_device->upload_filter(buffer, size);
            } else {
                wxMessageBox("No device connected!", "Run", wxICON_WARNING);
            }
        }
    }
}

void MainFrame::onRunUnload([[maybe_unused]] wxCommandEvent&)
{
    if (m_device != nullptr && m_device->connected())
        m_device->unload_filter();
}

void MainFrame::requestSamples()
{
    m_device_samples_future = std::async(std::launch::async,
                                         [this]() { return m_device->continuous_read(); });
}

