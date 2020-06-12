#ifndef WXAPP_HPP_
#define WXAPP_HPP_

#include "wxmain.hpp"

#include <wx/app.h>

class MainApp : public wxApp
{
public:
    virtual bool OnInit() final {
        auto mainFrame = new MainFrame;
        mainFrame->Show(true);
        SetTopWindow(mainFrame);
        return true;
    }
};

#endif // WXAPP_HPP_

