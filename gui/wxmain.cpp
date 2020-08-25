#include "wxmain.hpp"

#include <wx/filename.h>

void MainFrame::prepareEditor()
{
    m_text_editor->SetLexer(wxSTC_LEX_CPP);
    m_text_editor->SetMarginWidth(0, 30);
    m_text_editor->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    wxFont::AddPrivateFont("./Hack-Regular.ttf");
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
        wxT("void char short int long float double unsigned signed "
            "volatile static const constexpr constinit consteval "
            "virtual final noexcept public private protected"));
    m_text_editor->SetText("void process_data(adcsample_t *samples, unsigned int size)\n{\n\t\n}\n");
}

static const char *makefile_text = R"make(
all:
	@arm-none-eabi-g++ -x c++ -mcpu=cortex-m4 -mthumb -Os --specs=nosys.specs -nostartfiles -fPIE -c $0 -o $0.o
	@arm-none-eabi-ld -shared -n -N -z max-page-size=512 -Ttext-segment=0 \
	    $0.o -o $0.so
	@arm-none-eabi-strip -s -S --strip-unneeded $0.so
	@arm-none-eabi-objcopy --remove-section .dynsym \
						   --remove-section .dynstr \
						   --remove-section .dynamic \
						   --remove-section .hash \
						   --remove-section .ARM.exidx \
						   --remove-section .ARM.attributes \
						   --remove-section .comment \
						   $0.so
)make";

static const char *file_header = R"cpp(
#include <cstdint>

using adcsample_t = uint16_t;

__attribute__((section(".process_data"))) void process_data(adcsample_t *samples, unsigned int size);

// End stmdspgui header code

)cpp";

wxString MainFrame::compileEditorCode()
{
    auto file_text = wxString(file_header) + m_text_editor->GetText();
    auto file_name = wxFileName::CreateTempFileName("stmdspgui");
    wxFile file (file_name, wxFile::write);
    file.Write(file_text);
    file.Close();

    wxFile makefile (file_name + "make", wxFile::write);
    wxString make_text (makefile_text);
    make_text.Replace("$0", file_name);
    makefile.Write(make_text);
    makefile.Close();

    wxString make_command = wxString("make -C ") + file_name.BeforeLast('/') +
                            " -f " + file_name + "make";
    if (system(make_command.ToAscii()) == 0)
        return file_name + ".so";
    else
        return "";
}

