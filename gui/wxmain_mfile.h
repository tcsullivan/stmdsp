void MainFrame::onFileNew(wxCommandEvent&)
{
    m_open_file_path = "";
    m_text_editor->SetText(file_content);
    m_text_editor->DiscardEdits();
    m_status_bar->SetStatusText("Ready.");
}

void MainFrame::onFileOpen(wxCommandEvent&)
{
    wxFileDialog openDialog(this, "Open filter file", "", "",
                            "C++ source file (*.cpp)|*.cpp",
                            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openDialog.ShowModal() != wxID_CANCEL) {
        if (wxFileInputStream file_stream (openDialog.GetPath()); file_stream.IsOk()) {
            auto size = file_stream.GetSize();
            auto buffer = new char[size + 1];
            buffer[size] = '\0';
            if (file_stream.ReadAll(buffer, size)) {
                m_open_file_path = openDialog.GetPath();
                m_text_editor->SetText(buffer);
                m_text_editor->DiscardEdits();
                m_compile_output->ChangeValue("");
                m_status_bar->SetStatusText("Ready.");
            } else {
                m_status_bar->SetStatusText("Failed to read file contents.");
            }
            delete[] buffer;
        } else {
            m_status_bar->SetStatusText("Failed to open file.");
        }
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}

void MainFrame::onFileOpenTemplate(wxCommandEvent& event)
{
    auto file_path = wxGetCwd() + "/templates/" + m_menu_bar->GetLabel(event.GetId());

    if (wxFileInputStream file_stream (file_path); file_stream.IsOk()) {
        auto size = file_stream.GetSize();
        auto buffer = new char[size + 1];
        buffer[size] = '\0';
        if (file_stream.ReadAll(buffer, size)) {
            m_open_file_path = "";
            m_text_editor->SetText(buffer);
            //m_text_editor->DiscardEdits();
            m_status_bar->SetStatusText("Ready.");
        } else {
            m_status_bar->SetStatusText("Failed to read file contents.");
        }
        delete[] buffer;
    } else {
        m_status_bar->SetStatusText("Ready.");
    }
}


void MainFrame::onFileSave(wxCommandEvent& ce)
{
    if (m_text_editor->IsModified()) {
        if (m_open_file_path.IsEmpty()) {
            onFileSaveAs(ce);
        } else {
            if (wxFile file (m_open_file_path, wxFile::write); file.IsOpened()) {
                file.Write(m_text_editor->GetText());
                file.Close();
                m_text_editor->DiscardEdits();
                m_status_bar->SetStatusText("Saved.");
            } else {
                m_status_bar->SetStatusText("Save failed: couldn't open file.");
            }
        }
    } else {
        m_status_bar->SetStatusText("No modifications to save.");
    }
}

void MainFrame::onFileSaveAs(wxCommandEvent&)
{
    if (m_text_editor->IsModified()) {
        wxFileDialog saveDialog(this, "Save filter file", "", "",
                                "C++ source file (*.cpp)|*.cpp",
                                wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveDialog.ShowModal() != wxID_CANCEL) {
            if (wxFile file (saveDialog.GetPath(), wxFile::write); file.IsOpened()) {
                file.Write(m_text_editor->GetText());
                file.Close();
                m_text_editor->DiscardEdits();
                m_open_file_path = saveDialog.GetPath();
                m_status_bar->SetStatusText("Saved.");
            } else {
                m_status_bar->SetStatusText("Save failed: couldn't open file.");
            }
        }
    } else {
        m_status_bar->SetStatusText("No modifications to save.");
    }
}

void MainFrame::onFileQuit(wxCommandEvent&)
{
    Close(true);
}

