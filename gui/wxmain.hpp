#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include <wx/frame.h>
#include <wx/stattext.h>

class MainFrame : public wxFrame
{
public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 480))
    {
        auto lTitle = new wxStaticText(this, wxID_ANY, "Welcome to the GUI.");
    }
};

#endif // WXMAIN_HPP_

