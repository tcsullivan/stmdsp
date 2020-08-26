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

        auto mainFrame = new MainFrame;
        mainFrame->Show(true);
        SetTopWindow(mainFrame);
        return true;
    }
};

#endif // WXAPP_HPP_

