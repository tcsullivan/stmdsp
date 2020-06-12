#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include <wx/button.h>
#include <wx/frame.h>
#include <wx/stattext.h>

class MainFrame : public wxFrame
{
    enum Id {
        Welcome = 1,
        Single
    };

public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 480))
    {
        new wxStaticText(this, Id::Welcome, "Welcome to the GUI.", wxPoint(20, 20));
        new wxButton(this, Id::Single, "Single", wxPoint(20, 60));

        Bind(wxEVT_BUTTON, &MainFrame::onSinglePressed, this, Id::Single);
    }

    void onSinglePressed(wxCommandEvent& ce) {
        auto button = dynamic_cast<wxButton *>(ce.GetEventObject());
        button->SetLabel("Nice");
    }
};

#endif // WXMAIN_HPP_

