/**
 * @file wxapp.hpp
 * @brief Main application object for the stmdsp gui.
 *
 * Copyright (C) 2021 Clyne Sullivan
 *
 * Distributed under the GNU GPL v3 or later. You should have received a copy of
 * the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WXAPP_HPP_
#define WXAPP_HPP_

#include "wxmain.hpp"

#include <wx/app.h>
#include <wx/font.h>

class MainApp : public wxApp
{
public:
    virtual bool OnInit() final {
        //wxFont::AddPrivateFont("./Hack-Regular.ttf");

        m_main_frame = new MainFrame;
        m_main_frame->Show(true);
        SetTopWindow(m_main_frame);
        return true;
    }

private:
    MainFrame *m_main_frame = nullptr;
};

#endif // WXAPP_HPP_

