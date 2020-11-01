#include "wxmain.hpp"

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

#include <vector>

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
    MRunLogResults,
    MRunUpload,
    MRunUnload,
    MRunEditBSize,
    MRunEditSRate,
    MRunGenUpload,
    MRunGenStart,
    MCodeCompile,
    MCodeDisassemble
};

MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY, "stmdspgui", wxPoint(50, 50), wxSize(640, 800))
{
    auto splitter = new wxSplitterWindow(this, wxID_ANY);
    auto panelCode = new wxPanel(splitter, wxID_ANY);
    auto panelOutput = new wxPanel(splitter, wxID_ANY);
    auto sizerCode = new wxBoxSizer(wxVERTICAL);
    auto sizerOutput = new wxBoxSizer(wxVERTICAL);
    auto sizerSplitter = new wxBoxSizer(wxVERTICAL);
    auto menuFile = new wxMenu;
    auto menuRun = new wxMenu;
    auto menuCode = new wxMenu;

    // Member initialization
    m_status_bar = new wxStatusBar(this);
    m_text_editor = new wxStyledTextCtrl(panelCode, wxID_ANY, wxDefaultPosition, wxSize(620, 500));
    m_compile_output = new wxTextCtrl(panelOutput, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                      wxSize(620, 250), wxTE_READONLY | wxTE_MULTILINE | wxHSCROLL);
    m_measure_timer = new wxTimer(this, Id::MeasureTimer);
    m_menu_bar = new wxMenuBar;

    m_status_bar->SetStatusText("Ready.");
    SetStatusBar(m_status_bar);

    splitter->SetSashGravity(0.5);
    splitter->SetMinimumPaneSize(20);

    prepareEditor();
    sizerCode->Add(m_text_editor, 1, wxEXPAND | wxALL, 0);
    panelCode->SetSizer(sizerCode);

    m_compile_output->SetBackgroundColour(wxColour(0, 0, 0));
    m_compile_output->SetDefaultStyle(wxTextAttr(*wxWHITE, *wxBLACK, wxFont("Hack")));
    sizerOutput->Add(m_compile_output, 1, wxEXPAND | wxALL, 0);
    panelOutput->SetSizer(sizerOutput);

    splitter->SplitHorizontally(panelCode, panelOutput, 500);

    sizerSplitter->Add(splitter, 1, wxEXPAND, 5);
    SetSizer(sizerSplitter);
    sizerSplitter->SetSizeHints(this);

    Bind(wxEVT_TIMER, &MainFrame::onMeasureTimer, this, Id::MeasureTimer);

    Bind(wxEVT_MENU, &MainFrame::onFileNew, this, Id::MFileNew, wxID_ANY,
         menuFile->Append(MFileNew, "&New"));
    Bind(wxEVT_MENU, &MainFrame::onFileOpen, this, Id::MFileOpen, wxID_ANY,
         menuFile->Append(MFileOpen, "&Open"));

    menuFile->Append(MFileOpenTemplate, "Open &Template", loadTemplates());

    Bind(wxEVT_MENU, &MainFrame::onFileSave, this, Id::MFileSave, wxID_ANY,
         menuFile->Append(MFileSave, "&Save"));
    Bind(wxEVT_MENU, &MainFrame::onFileSaveAs, this, Id::MFileSaveAs, wxID_ANY,
         menuFile->Append(MFileSaveAs, "Save &As"));
    menuFile->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onFileQuit, this, Id::MFileQuit, wxID_ANY,
         menuFile->Append(MFileQuit, "&Quit"));

    Bind(wxEVT_MENU, &MainFrame::onRunConnect, this, Id::MRunConnect, wxID_ANY,
         menuRun->Append(MRunConnect, "&Connect"));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunStart, this, Id::MRunStart, wxID_ANY,
         menuRun->Append(MRunStart, "&Start"));
    m_run_measure = menuRun->AppendCheckItem(MRunMeasure, "&Measure code time");
    Bind(wxEVT_MENU, &MainFrame::onRunLogResults, this, Id::MRunLogResults, wxID_ANY,
         menuRun->AppendCheckItem(MRunLogResults, "&Log results..."));
    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunUpload, this, Id::MRunUpload, wxID_ANY,
         menuRun->Append(MRunUpload, "&Upload code"));
    Bind(wxEVT_MENU, &MainFrame::onRunUnload, this, Id::MRunUnload, wxID_ANY,
         menuRun->Append(MRunUnload, "U&nload code"));
    Bind(wxEVT_MENU, &MainFrame::onRunEditBSize, this, Id::MRunEditBSize, wxID_ANY,
         menuRun->Append(MRunEditBSize, "Set &buffer size..."));
    Bind(wxEVT_MENU, &MainFrame::onRunEditSRate, this, Id::MRunEditSRate, wxID_ANY,
         menuRun->Append(MRunEditSRate, "Set sample &rate..."));

    menuRun->AppendSeparator();
    Bind(wxEVT_MENU, &MainFrame::onRunGenUpload, this, Id::MRunGenUpload, wxID_ANY,
         menuRun->Append(MRunGenUpload, "&Load signal generator..."));
    Bind(wxEVT_MENU, &MainFrame::onRunGenStart, this, Id::MRunGenStart, wxID_ANY,
         menuRun->AppendCheckItem(MRunGenStart, "Start &generator"));

    Bind(wxEVT_MENU, &MainFrame::onRunCompile, this, Id::MCodeCompile, wxID_ANY,
         menuCode->Append(MCodeCompile, "&Compile code"));
    Bind(wxEVT_MENU, &MainFrame::onCodeDisassemble, this, Id::MCodeDisassemble, wxID_ANY,
         menuCode->Append(MCodeDisassemble, "Show &Disassembly"));

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::onCloseEvent, this, wxID_ANY);

    m_menu_bar->Append(menuFile, "&File");
    m_menu_bar->Append(menuRun, "&Run");
    m_menu_bar->Append(menuCode, "&Code");
    SetMenuBar(m_menu_bar);
}

void MainFrame::onCloseEvent(wxCloseEvent& event)
{
    SetMenuBar(nullptr);
    m_menu_bar->Remove(2);
    m_menu_bar->Remove(1);
    m_menu_bar->Remove(0);
    delete m_menu_bar;
    delete m_measure_timer;

    event.Skip();
}

void MainFrame::onMeasureTimer([[maybe_unused]] wxTimerEvent&)
{
    if (m_conv_result_log != nullptr) {
        if (auto samples = m_device->continuous_read(); samples.size() > 0) {
            for (auto& s : samples) {
                auto str = wxString::Format("%u\n", s);
                m_conv_result_log->Write(str.ToAscii(), str.Len());
            }
        }
    } else if (m_status_bar && m_run_measure && m_run_measure->IsChecked()) {
        m_status_bar->SetStatusText(wxString::Format(wxT("Execution time: %u cycles"),
                                                     m_device->continuous_start_get_measurement()));
    }
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
constexpr unsigned int SIZE = 4000;

adcsample_t *process_data(adcsample_t *samples, unsigned int size);

extern "C" void process_data_entry()
{
    ((void (*)())process_data)();
}

// End stmdspgui header code

)cpp");

wxString MainFrame::compileEditorCode()
{
    if (m_temp_file_name.IsEmpty())
        m_temp_file_name = wxFileName::CreateTempFileName("stmdspgui");

    wxFile file (m_temp_file_name, wxFile::write);
    file.Write(file_header + m_text_editor->GetText());
    file.Close();

    wxFile makefile (m_temp_file_name + "make", wxFile::write);
    wxString make_text (makefile_text);
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
        m_compile_output->ChangeValue("");
        return m_temp_file_name + ".o";
    } else {
        m_status_bar->SetStatusText("Compilation failed.");
        m_compile_output->LoadFile(make_output);
        return "";
    }
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
    m_status_bar->SetStatusText("Ready.");
}

void MainFrame::onFileOpen([[maybe_unused]] wxCommandEvent&)
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
                m_status_bar->SetStatusText("Ready.");
            }
            delete[] buffer;
        }
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
        }
        delete[] buffer;
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
                m_status_bar->SetStatusText("Saved.");
            } else {
                m_status_bar->SetStatusText("Save failed: couldn't open file.");
            }
        }
    } else {
        m_status_bar->SetStatusText("No modifications to save.");
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
        menuItem->SetItemLabel("&Connect");
        m_status_bar->SetStatusText("Disconnected.");
    }
}

void MainFrame::onRunStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_is_running) {
        if (m_device != nullptr && m_device->connected()) {
            if (m_run_measure && m_run_measure->IsChecked()) {
                m_device->continuous_start_measure();
                m_measure_timer->StartOnce(1000);
            } else {
                m_device->continuous_start();
                m_measure_timer->Start(15);
            }

            menuItem->SetItemLabel("&Stop");
            m_status_bar->SetStatusText("Running.");
            m_is_running = true;
        } else {
            wxMessageBox("No device connected!", "Run", wxICON_WARNING);
            m_status_bar->SetStatusText("Please connect.");
        }
    } else {
        m_device->continuous_stop();

        menuItem->SetItemLabel("&Start");
        m_status_bar->SetStatusText("Ready.");
        m_is_running = false;
    }
}

void MainFrame::onRunLogResults(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (menuItem->IsChecked()) {
        wxFileDialog dialog (this, "Choose log file", "", "", "*.csv",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_CANCEL) {
            if (m_conv_result_log != nullptr) {
                m_conv_result_log->Close();
                delete m_conv_result_log;
                m_conv_result_log = nullptr;
            }

            m_conv_result_log = new wxFileOutputStream(dialog.GetPath());
        }

        m_status_bar->SetStatusText("Ready.");
    } else if (m_conv_result_log != nullptr) {
        m_conv_result_log->Close();
        delete m_conv_result_log;
        m_conv_result_log = nullptr;
    }
}

void MainFrame::onRunEditBSize([[maybe_unused]] wxCommandEvent&)
{
    if (m_device != nullptr && m_device->connected()) {
        wxTextEntryDialog dialog (this, "Enter new buffer size (100-4000)", "Set Buffer Size");
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
    } else {
        wxMessageBox("No device connected!", "Run", wxICON_WARNING);
        m_status_bar->SetStatusText("Please connect.");
    }
}

void MainFrame::onRunEditSRate([[maybe_unused]] wxCommandEvent&)
{
    if (m_device != nullptr && m_device->connected()) {
        wxTextEntryDialog dialog (this, "Enter new sample rate id:", "Set Sample Rate");
        if (dialog.ShowModal() == wxID_OK) {
            if (wxString value = dialog.GetValue(); !value.IsEmpty()) {
                if (unsigned long n; value.ToULong(&n)) {
                    if (n < 20) {
                        m_device->set_sample_rate(n);
                    } else {
                        m_status_bar->SetStatusText("Error: Invalid sample rate.");
                    }
                } else {
                    m_status_bar->SetStatusText("Error: Invalid sample rate.");
                }
            } else {
                m_status_bar->SetStatusText("Ready.");
            }
        } else {
            m_status_bar->SetStatusText("Ready.");
        }
    } else {
        wxMessageBox("No device connected!", "Run", wxICON_WARNING);
        m_status_bar->SetStatusText("Please connect.");
    }
}

void MainFrame::onRunGenUpload([[maybe_unused]] wxCommandEvent&)
{
    if (m_device != nullptr && m_device->connected()) {
        wxTextEntryDialog dialog (this, "Enter generator values below. Values must be whole numbers "
                                        "between zero and 4095.", "Enter Generator Values");
        if (dialog.ShowModal() == wxID_OK) {
            if (wxString values = dialog.GetValue(); !values.IsEmpty()) {
                std::vector<stmdsp::dacsample_t> samples;
                while (!values.IsEmpty()) {
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
                    m_status_bar->SetStatusText("Error: Too many samples.");
                }
            } else {
                m_status_bar->SetStatusText("Error: No samples given.");
            }
        } else {
            m_status_bar->SetStatusText("Ready.");
        }
    } else {
        wxMessageBox("No device connected!", "Run", wxICON_WARNING);
        m_status_bar->SetStatusText("Please connect.");
    }
}

void MainFrame::onRunGenStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (m_device != nullptr && m_device->connected()) {
        if (menuItem->IsChecked()) {
            m_device->siggen_start();
            menuItem->SetItemLabel("Stop &generator");
        } else {
            m_device->siggen_stop();
            menuItem->SetItemLabel("Start &generator");
        }
    } else {
        wxMessageBox("No device connected!", "Run", wxICON_WARNING);
        m_status_bar->SetStatusText("Please connect.");
    }
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
                m_status_bar->SetStatusText("Code uploaded.");
            } else {
                wxMessageBox("No device connected!", "Run", wxICON_WARNING);
                m_status_bar->SetStatusText("Please connect.");
            }
        } else {
             m_status_bar->SetStatusText("Couldn't load compiled code.");
        }
    }
}

void MainFrame::onRunUnload([[maybe_unused]] wxCommandEvent&)
{
    if (m_device != nullptr && m_device->connected()) {
        m_device->unload_filter();
        m_status_bar->SetStatusText("Unloaded code.");
    } else {
        m_status_bar->SetStatusText("No device connected.");
    }
}

void MainFrame::onRunCompile([[maybe_unused]] wxCommandEvent&)
{
    compileEditorCode();
}

void MainFrame::onCodeDisassemble([[maybe_unused]] wxCommandEvent&)
{
    auto output = m_temp_file_name + ".asm.log";
    wxString command = wxString("arm-none-eabi-objdump -d --no-show-raw-insn ") + m_temp_file_name + ".orig.o"
                                " > " + output + " 2>&1";

    if (system(command.ToAscii()) == 0) {
        m_compile_output->LoadFile(output);
        m_status_bar->SetStatusText(wxString::Format(wxT("Done. Line count: %u."),
                                                         m_compile_output->GetNumberOfLines()));
    } else {
        m_compile_output->ChangeValue("");
        m_status_bar->SetStatusText("Failed to load disassembly (code compiled?).");
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

