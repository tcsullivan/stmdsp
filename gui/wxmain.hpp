#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/frame.h>
#include <wx/stattext.h>

class MainFrame : public wxFrame
{
    enum Id {
        Welcome = 1,
        Single
    };

    int m_radius = 10;

    const wxRect m_clipping_region = {20, 100, 600, 360};

public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 480))
    {
        new wxStaticText(this, Id::Welcome, "Welcome to the GUI.", wxPoint(20, 20));
        new wxButton(this, Id::Single, "Single", wxPoint(20, 60));

        Bind(wxEVT_BUTTON, &MainFrame::onSinglePressed, this, Id::Single);
        Bind(wxEVT_PAINT, &MainFrame::onPaint, this, wxID_ANY);
    }

    void onPaint([[maybe_unused]] wxPaintEvent& pe) {
        auto *dc = new wxClientDC(this);
        dc->SetClippingRegion(m_clipping_region);

        dc->SetBrush(*wxBLACK_BRUSH);
        dc->SetPen(*wxBLACK_PEN);
        dc->DrawRectangle(m_clipping_region);

        dc->SetPen(*wxRED_PEN);
        dc->DrawCircle(320, 240, m_radius);
    }

    void onSinglePressed(wxCommandEvent& ce) {
        auto button = dynamic_cast<wxButton *>(ce.GetEventObject());
        button->SetLabel("Nice");
        updateDrawing();
    }

    void updateDrawing() {
        m_radius += 10;
        this->RefreshRect(m_clipping_region);
    }
};

#endif // WXMAIN_HPP_

