#ifndef WXMAIN_HPP_
#define WXMAIN_HPP_

#include "stmdsp.hpp"

#include <future>
#include <thread>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/stattext.h>
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

    const wxRect m_clipping_region = {20, 100, 600, 360};

    stmdsp::device *m_device = nullptr;
    std::future<std::vector<stmdsp::adcsample_t>> m_device_samples_future;
    std::vector<stmdsp::adcsample_t> m_device_samples;

public:
    MainFrame() : wxFrame(nullptr, -1, "Hello world", wxPoint(50, 50), wxSize(640, 480))
    {
        new wxStaticText(this, Id::Welcome, "Welcome to the GUI.", wxPoint(20, 20));
        new wxButton(this, Id::Single, "Single", wxPoint(20, 60));
        new wxButton(this, Id::UploadFilter, "Upload Filter", wxPoint(120, 60));
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

