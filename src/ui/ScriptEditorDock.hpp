#pragma once

#include "vendor/TextEditor/TextEditor.h"
#include <string>
#include <vector>
#include <memory>

namespace Djusov {

class ScriptEditorDock {
public:
    ScriptEditorDock();

    void init();
    void render(bool* p_open = nullptr);

    void openFile(const std::string& filePath);
    void newScript();
    void saveCurrentFile();

    const std::string& getCurrentFilePath() const { return m_currentFilePath; }

private:
    TextEditor m_editor;
    std::string m_currentFilePath;
    std::string m_statusMessage;
    bool m_isDirty;
    bool m_showAutocomplete;
    std::string m_currentWord;
    std::vector<std::string> m_autocompleteCandidates;
};

} // namespace Djusov
