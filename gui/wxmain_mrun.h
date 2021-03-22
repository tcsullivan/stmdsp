void MainFrame::onRunConnect(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_device) {
        stmdsp::scanner scanner;
        if (auto devices = scanner.scan(); devices.size() > 0) {
            m_device = new stmdsp::device(devices.front());
            if (m_device->connected()) {
                auto rate = m_device->get_sample_rate();
                m_rate_select->SetSelection(rate);

                updateMenuOptions();
                menuItem->SetItemLabel("&Disconnect");
                m_status_bar->SetStatusText("Connected.");
            } else {
                delete m_device;
                m_device = nullptr;

                menuItem->SetItemLabel("&Connect");
                m_status_bar->SetStatusText("Failed to connect.");
            }
        } else {
            m_status_bar->SetStatusText("No devices found.");
        }
    } else {
        delete m_device;
        m_device = nullptr;
        updateMenuOptions();
        menuItem->SetItemLabel("&Connect");
        m_status_bar->SetStatusText("Disconnected.");
    }
}

void MainFrame::onRunStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());

    if (!m_is_running) {
        if (m_run_measure->IsChecked()) {
            m_device->continuous_start_measure();
            m_measure_timer->StartOnce(1000);
        } else {
            if (m_device->is_siggening() && m_wav_clip) {
                m_measure_timer->Start(m_device->get_buffer_size() * 500 / 
                                       srateNums[m_rate_select->GetSelection()]);
            } else if (m_conv_result_log) {
                m_measure_timer->Start(15);
            } else if (m_run_draw_samples->IsChecked()) {
                m_measure_timer->Start(300);
            }

            m_device->continuous_start();
        }

        m_rate_select->Enable(false);
        menuItem->SetItemLabel("&Stop");
        m_status_bar->SetStatusText("Running.");
        m_is_running = true;
    } else {
        m_device->continuous_stop();
        m_measure_timer->Stop();

        m_rate_select->Enable(true);
        menuItem->SetItemLabel("&Start");
        m_status_bar->SetStatusText("Ready.");
        m_is_running = false;

        if (m_run_draw_samples->IsChecked())
            m_compile_output->Refresh();
    }
}

void MainFrame::onRunLogResults(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (menuItem->IsChecked()) {
        wxFileDialog dialog (this, "Choose log file", "", "", "*.csv",
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_CANCEL) {
            if (m_conv_result_log) {
                m_conv_result_log->Close();
                delete m_conv_result_log;
                m_conv_result_log = nullptr;
            }

            m_conv_result_log = new wxFileOutputStream(dialog.GetPath());
        }

        m_status_bar->SetStatusText("Ready.");
    } else if (m_conv_result_log) {
        m_conv_result_log->Close();
        delete m_conv_result_log;
        m_conv_result_log = nullptr;
    }
}

void MainFrame::onRunEditBSize(wxCommandEvent&)
{
    wxTextEntryDialog dialog (this, "Enter new buffer size (100-4096)", "Set Buffer Size");
    if (dialog.ShowModal() == wxID_OK) {
        if (wxString value = dialog.GetValue(); !value.IsEmpty()) {
            if (unsigned long n; value.ToULong(&n)) {
                if (n >= 100 && n <= stmdsp::SAMPLES_MAX) {
                    m_device->continuous_set_buffer_size(n);
                } else {
                    m_status_bar->SetStatusText("Error: Invalid buffer size.");
                }
            } else {
                m_status_bar->SetStatusText("Error: Invalid buffer size.");
            }
        } else {
            m_status_bar->SetStatusText("Ready.");
        }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onRunGenUpload(wxCommandEvent&)
{
    if (SiggenDialog dialog (this); dialog.ShowModal() == wxID_OK) {
        auto result = dialog.Result();
        if (result.Find(".wav") != wxNOT_FOUND) {
            // Audio
            m_wav_clip = new wav::clip(result/*.Mid(1)*/);
            if (m_wav_clip->valid()) {
                m_status_bar->SetStatusText("Generator ready.");
            } else {
                delete m_wav_clip;
                m_wav_clip = nullptr;
                m_status_bar->SetStatusText("Error: Bad WAV file.");
            }
        } else if (result.find_first_not_of("0123456789 \t\r\n") == wxString::npos) {
            // List
            std::vector<stmdsp::dacsample_t> samples;
            while (!result.IsEmpty() && samples.size() <= stmdsp::SAMPLES_MAX * 2) {
                if (auto number_end = result.find_first_not_of("0123456789");
                    number_end != wxString::npos && number_end > 0)
                {
                    auto number = result.Left(number_end);
                    if (unsigned long n; number.ToULong(&n))
                        samples.push_back(n & 4095);

                    if (auto next = result.find_first_of("0123456789", number_end + 1);
                        next != wxString::npos)
                    {
                        result = result.Mid(next);
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }

            if (samples.size() <= stmdsp::SAMPLES_MAX * 2) {
                m_device->siggen_upload(&samples[0], samples.size());
                m_status_bar->SetStatusText("Generator ready.");
            } else {
                m_status_bar->SetStatusText(wxString::Format("Error: Too many samples (max is %u).",
                                                             stmdsp::SAMPLES_MAX * 2));
            }
        } else {
            // Formula
            m_status_bar->SetStatusText("Sorry, formulas not supported yet.");
        }
    //    if (wxString values = dialog.GetValue(); !values.IsEmpty()) {
    //        if (values[0] == '/') {
    //            m_wav_clip = new wav::clip(values.Mid(1));
    //            if (m_wav_clip->valid()) {
    //                m_status_bar->SetStatusText("Generator ready.");
    //            } else {
    //                delete m_wav_clip;
    //                m_wav_clip = nullptr;
    //                m_status_bar->SetStatusText("Error: Bad WAV file.");
    //            }
    //        } else {
    //        }
    //    } else {
    //        m_status_bar->SetStatusText("Error: No samples given.");
    //    }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onRunGenStart(wxCommandEvent& ce)
{
    auto menuItem = dynamic_cast<wxMenuItem *>(ce.GetEventUserData());
    if (menuItem->IsChecked()) {
        m_device->siggen_start();
        menuItem->SetItemLabel("Stop &generator");
        m_status_bar->SetStatusText("Generator running.");
    } else {
        m_device->siggen_stop();
        menuItem->SetItemLabel("Start &generator");
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onRunUpload(wxCommandEvent&)
{
    if (auto file = compileEditorCode(); !file.IsEmpty()) {
        if (wxFileInputStream file_stream (file); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new unsigned char[size];
            file_stream.ReadAll(buffer, size);
            m_device->upload_filter(buffer, size);
            m_status_bar->SetStatusText("Code uploaded.");
        } else {
             m_status_bar->SetStatusText("Couldn't load compiled code.");
        }
    }
}

void MainFrame::onRunUnload(wxCommandEvent&)
{
    m_device->unload_filter();
    m_status_bar->SetStatusText("Unloaded code.");
}

void MainFrame::onRunCompile(wxCommandEvent&)
{
    compileEditorCode();
}

