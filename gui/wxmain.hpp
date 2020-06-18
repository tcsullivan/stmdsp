#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"

#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/timer.h>

class MainFrame : public wxFrame
{
    enum Id {
        Welcome = 1,
        Single,
        SelectDevice,
        RenderTimer
    };

    bool m_is_rendering = false;
    wxTimer *m_render_timer = nullptr;
    int m_radius = 10;

    const wxRect m_clipping_region = {20, 100, 600, 360};

public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 480))
    {
        new wxStaticText(this, Id::Welcome, "Welcome to the GUI.", wxPoint(20, 20));
        new wxButton(this, Id::Single, "Single", wxPoint(20, 60));
        auto combo = new wxComboBox(this, Id::SelectDevice, "", wxPoint(470, 20), wxSize(150, 30));
        combo->SetEditable(false);
        stmdsp::scanner scanner;
        for (auto& dev : scanner.scan())
            combo->Append(dev);
        if (combo->GetCount() > 0)
            combo->SetSelection(0);

        m_render_timer = new wxTimer(this, Id::RenderTimer);

        Bind(wxEVT_BUTTON, &MainFrame::onSinglePressed, this, Id::Single);
        Bind(wxEVT_PAINT, &MainFrame::onPaint, this, wxID_ANY);
        Bind(wxEVT_TIMER, &MainFrame::onRenderTimer, this, Id::RenderTimer);
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

        if (!m_render_timer->IsRunning()) {
            m_render_timer->Start(100);
            button->SetLabel("Stop");
        } else {
            m_render_timer->Stop();
            button->SetLabel("Single");
        }
    }

    void onRenderTimer([[maybe_unused]] wxTimerEvent& te) {
        updateDrawing();
    }

    void updateDrawing() {
        m_radius++;
        this->RefreshRect(m_clipping_region);
    }
};

#endif // WXMAIN_HPP_

