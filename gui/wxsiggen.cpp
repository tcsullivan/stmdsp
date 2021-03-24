/**
 * @file wxsiggen.cpp
 * @brief Dialog prompt for providing signal generator input.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "wxsiggen.hpp"

#include <wx/filedlg.h>
#include <wx/radiobox.h>

static const std::array<wxString, 3> Sources {{
    "List",
    "Formula",
    "WAV audio"
}};
static const std::array<wxString, 3> Instructions {{
    "Enter a list of numbers:",
    "Enter a formula. f(x) = ",
    wxEmptyString
}};

SiggenDialog::SiggenDialog(wxWindow *parent) :
    wxDialog(parent, wxID_ANY, "stmdspgui signal generator", wxDefaultPosition, wxSize(300, 200))
{
    m_instruction = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(10, 70));
    m_source_list = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxPoint(10, 100), wxSize(280, 30));
    m_source_math = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxPoint(10, 100), wxSize(280, 30));
    m_source_file = new wxButton(this, 42, "Choose file...", wxPoint(10, 75), wxSize(280, 50));

    auto radio = new wxRadioBox(this, wxID_ANY, "Source", wxPoint(10, 10), wxSize(280, 50),
                                Sources.size(), Sources.data());
    auto save = new wxButton(this, 43, "Save", wxPoint(200, 150));

    m_instruction->SetLabel(Instructions[0]);
    m_source_math->Hide();
    m_source_file->Hide();

    Bind(wxEVT_RADIOBOX, &SiggenDialog::onSourceChange, this, wxID_ANY, wxID_ANY, radio);
    Bind(wxEVT_BUTTON, &SiggenDialog::onSourceFile, this, 42, 42, m_source_file);
    Bind(wxEVT_BUTTON, &SiggenDialog::onSave, this, 43, 43, save);
}

SiggenDialog::~SiggenDialog()
{
    Unbind(wxEVT_BUTTON, &SiggenDialog::onSave, this, 43, 43);
    Unbind(wxEVT_BUTTON, &SiggenDialog::onSourceFile, this, 42, 42);
    Unbind(wxEVT_RADIOBOX, &SiggenDialog::onSourceChange, this, wxID_ANY, wxID_ANY);
}

void SiggenDialog::onSourceFile(wxCommandEvent&)
{
    wxFileDialog dialog (this, "Open audio file", "", "",
                         "Audio file (*.wav)|*.wav",
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_CANCEL)
        m_result = dialog.GetPath();
}

void SiggenDialog::onSave(wxCommandEvent&)
{
    if (m_source_list->IsShown())
        m_result = m_source_list->GetValue();
    else if (m_source_math->IsShown())
        m_result = m_source_math->GetValue();

    EndModal(!m_result.IsEmpty() ? wxID_OK : wxID_CANCEL);
}

void SiggenDialog::onSourceChange(wxCommandEvent& ce)
{
    auto radio = dynamic_cast<wxRadioBox*>(ce.GetEventObject());
    if (radio == nullptr)
        return;

    m_result.Clear();
    if (unsigned int selection = static_cast<unsigned int>(radio->GetSelection());
        selection < Sources.size())
    {
        m_instruction->SetLabel(Instructions[selection]);
        m_source_list->Show(selection == 0);
        m_source_math->Show(selection == 1);
        m_source_file->Show(selection == 2);
    }
}

