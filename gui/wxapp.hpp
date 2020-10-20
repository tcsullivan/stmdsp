#ifndef WXAPP_HPP_
#define WXAPP_HPP_

#include "wxmain.hpp"

#include <wx/app.h>
#include <wx/font.h>

class MainApp : public wxApp
{
public:
    virtual bool OnInit() final {
        wxFont::AddPrivateFont("./Hack-Regular.ttf");

        m_main_frame = new MainFrame;
        m_main_frame->Show(true);
        SetTopWindow(m_main_frame);
        return true;
    }

private:
    MainFrame *m_main_frame = nullptr;
};

#endif // WXAPP_HPP_

