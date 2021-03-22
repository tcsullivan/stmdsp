/**
 * @file wxsiggen.hpp
 * @brief Dialog prompt for providing signal generator input.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WXSIGGEN_HPP_
#define WXSIGGEN_HPP_

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

class SiggenDialog : public wxDialog {
public:
    SiggenDialog(wxWindow *parent);
    ~SiggenDialog();

    auto Result() const {
        return m_result;
    }

    void onSourceChange(wxCommandEvent&);
    void onSourceFile(wxCommandEvent&);
    void onSave(wxCommandEvent&);

private:
    wxStaticText *m_instruction = nullptr;
    wxTextCtrl *m_source_list = nullptr;
    wxTextCtrl *m_source_math = nullptr;
    wxButton *m_source_file = nullptr;
    wxString m_result;
};

#endif // WXSIGGEN_HPP_

