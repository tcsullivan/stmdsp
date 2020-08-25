#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"

#include <future>
#include <thread>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/stc/stc.h>
#include <wx/timer.h>
#include <wx/wfstream.h>

class MainFrame : public wxFrame
{
    enum Id {
        Welcome = 1,
        Single,
        SelectDevice,
        UploadFilter,
        RenderTimer
    };

    bool m_is_rendering = false;
    wxTimer *m_render_timer = nullptr;
    wxComboBox *m_device_combo = nullptr;

    const wxRect m_clipping_region = {20, 500, 600, 360};

    stmdsp::device *m_device = nullptr;
    std::future<std::vector<stmdsp::adcsample_t>> m_device_samples_future;
    std::vector<stmdsp::adcsample_t> m_device_samples;

public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 880))
    {
        new wxStaticText(this, Id::Welcome, "Welcome to the GUI.", wxPoint(20, 20));
        new wxButton(this, Id::Single, "Single", wxPoint(20, 60));
        new wxButton(this, Id::UploadFilter, "Upload Filter", wxPoint(120, 60));
        auto stc = new wxStyledTextCtrl(this, wxID_ANY, wxPoint(20, 100), wxSize(600, 400));

        stc->SetLexer(wxSTC_LEX_CPP);
        stc->SetMarginWidth(0, 30);
        stc->SetMarginType(0, wxSTC_MARGIN_NUMBER);
        wxFont::AddPrivateFont("./Hack-Regular.ttf");
        stc->StyleSetFaceName(wxSTC_STYLE_DEFAULT, "Hack");
        stc->StyleClearAll();
        stc->SetTabWidth(4);
        stc->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColor(75, 75, 75));
        stc->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColor(220, 220, 220));

        stc->StyleSetForeground (wxSTC_C_STRING,            wxColour(150,0,0));
        stc->StyleSetForeground (wxSTC_C_PREPROCESSOR,      wxColour(165,105,0));
        stc->StyleSetForeground (wxSTC_C_IDENTIFIER,        wxColour(40,0,60));
        stc->StyleSetForeground (wxSTC_C_NUMBER,            wxColour(0,150,0));
        stc->StyleSetForeground (wxSTC_C_CHARACTER,         wxColour(150,0,0));
        stc->StyleSetForeground (wxSTC_C_WORD,              wxColour(0,0,150));
        stc->StyleSetForeground (wxSTC_C_WORD2,             wxColour(0,150,0));
        stc->StyleSetForeground (wxSTC_C_COMMENT,           wxColour(150,150,150));
        stc->StyleSetForeground (wxSTC_C_COMMENTLINE,       wxColour(150,150,150));
        stc->StyleSetForeground (wxSTC_C_COMMENTDOC,        wxColour(150,150,150));
        stc->StyleSetForeground (wxSTC_C_COMMENTDOCKEYWORD, wxColour(0,0,200));
        stc->StyleSetForeground (wxSTC_C_COMMENTDOCKEYWORDERROR, wxColour(0,0,200));
        stc->StyleSetBold(wxSTC_C_WORD, true);
        stc->StyleSetBold(wxSTC_C_WORD2, true);
        stc->StyleSetBold(wxSTC_C_COMMENTDOCKEYWORD, true);
 
        // a sample list of keywords, I haven't included them all to keep it short...
        stc->SetKeyWords(0,
            wxT("return for while do break continue if else goto"));
        stc->SetKeyWords(1,
            wxT("void char short int long float double unsigned signed "
                "volatile static const constexpr constinit consteval "
                "virtual final noexcept public private protected"));

        m_device_combo = new wxComboBox(this, Id::SelectDevice, "", wxPoint(470, 20), wxSize(150, 30));
        m_device_combo->SetEditable(false);
        stmdsp::scanner scanner;
        for (auto& dev : scanner.scan())
            m_device_combo->Append(dev);
        if (m_device_combo->GetCount() > 0)
            m_device_combo->SetSelection(0);

        m_render_timer = new wxTimer(this, Id::RenderTimer);

        Bind(wxEVT_BUTTON, &MainFrame::onSinglePressed, this, Id::Single);
        Bind(wxEVT_BUTTON, &MainFrame::onUploadPressed, this, Id::UploadFilter);
        Bind(wxEVT_PAINT, &MainFrame::onPaint, this, wxID_ANY);
        Bind(wxEVT_TIMER, &MainFrame::onRenderTimer, this, Id::RenderTimer);
    }

    void onPaint([[maybe_unused]] wxPaintEvent& pe) {
        auto *dc = new wxClientDC(this);
        dc->SetClippingRegion(m_clipping_region);

        dc->SetBrush(*wxBLACK_BRUSH);
        dc->SetPen(*wxBLACK_PEN);
        dc->DrawRectangle(m_clipping_region);

        if (m_device_samples.size() > 0) {
            dc->SetPen(*wxRED_PEN);
            auto points = new wxPoint[m_device_samples.size()];
            const float spacing = static_cast<float>(m_clipping_region.GetWidth()) / m_device_samples.size();
            float x = 0;
            for (auto ptr = points; auto sample : m_device_samples) {
                *ptr++ = wxPoint {
                    static_cast<int>(x),
                    m_clipping_region.GetHeight() - sample * m_clipping_region.GetHeight() / 4096
                };
                x += spacing;
            }
            dc->DrawLines(m_device_samples.size(), points, m_clipping_region.GetX(), m_clipping_region.GetY());
            delete[] points;
        }
    }

    void doSingle() {
        m_device_samples_future = std::async(std::launch::async,
                                             [this]() { return m_device->continuous_read(); });
    }

    void onSinglePressed(wxCommandEvent& ce) {
        auto button = dynamic_cast<wxButton *>(ce.GetEventObject());

        if (!m_render_timer->IsRunning()) {
            m_device = new stmdsp::device(m_device_combo->GetStringSelection().ToStdString());
            if (m_device->connected()) {
                m_device->continuous_start();
                m_device_samples_future = std::async(std::launch::async,
                                                     []() { return decltype(m_device_samples)(); });
                m_render_timer->Start(1000);
                button->SetLabel("Stop");
            } else {
                delete m_device;
                m_device = nullptr;
            }
        } else {
            m_render_timer->Stop();
            m_device->continuous_stop();
            button->SetLabel("Single");

            delete m_device;
            m_device = nullptr;
        }
    }

    void onUploadPressed(wxCommandEvent& ce) {
        wxFileDialog dialog (this, "Select filter to upload", "", "", "*.so",
                             wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dialog.ShowModal() != wxID_CANCEL) {
            if (wxFileInputStream file_stream (dialog.GetPath()); file_stream.IsOk()) {
                auto size = file_stream.GetSize();
                auto buffer = new unsigned char[size];
                auto device = new stmdsp::device(m_device_combo->GetStringSelection().ToStdString());
                if (device->connected()) {
                    file_stream.ReadAll(buffer, size);
                    device->upload_filter(buffer, size);
                }
            }
        }
    }

    void onRenderTimer([[maybe_unused]] wxTimerEvent& te) {
        updateDrawing();
    }

    void updateDrawing() {
        if (m_device_samples = m_device_samples_future.get(); m_device_samples.size() > 0)
            this->RefreshRect(m_clipping_region);

        doSingle();
    }
};

#endif // WXMAIN_HPP_

