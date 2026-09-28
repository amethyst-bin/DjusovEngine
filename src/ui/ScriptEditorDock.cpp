#include "ui/ScriptEditorDock.hpp"
#include "script/ScriptEngine.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace Djusov {

ScriptEditorDock::ScriptEditorDock()
    : m_currentFilePath("de/scripts/main.luau"),
      m_statusMessage("Ready"),
      m_isDirty(false),
      m_showAutocomplete(false) {}

void ScriptEditorDock::init() {
    auto lang = TextEditor::LanguageDefinition::Lua();

    // Add Luau specific keywords & types
    lang.mKeywords.insert("type");
    lang.mKeywords.insert("export");
    lang.mKeywords.insert("continue");

    static const char* const luauTypes[] = {
        "number", "string", "boolean", "table", "any", "nil", "thread", "buffer", "Vector3", "CFrame"
    };
    for (auto& t : luauTypes) {
        TextEditor::Identifier id;
        id.mDeclaration = "Luau Type";
        lang.mIdentifiers.insert(std::make_pair(std::string(t), id));
    }

    // Add @de/... module identifiers
    static const char* const deModules[] = {
        "SaveManager", "World", "Players", "Lighting", "TweenService", "Input", "Physics"
    };
    for (auto& m : deModules) {
        TextEditor::Identifier id;
        id.mDeclaration = "DjusovEngine Service Module";
        lang.mIdentifiers.insert(std::make_pair(std::string(m), id));
    }

    m_editor.SetLanguageDefinition(lang);
    m_editor.SetPalette(TextEditor::GetDarkPalette());
    m_editor.SetShowWhitespaces(false);

    // Initial default script
    std::string defaultCode = 
        "-- DjusovEngine Luau Script\n"
        "local World = require(\"@de/world\")\n"
        "local Players = require(\"@de/players\")\n"
        "local SaveManager = require(\"@de/savemanager\")\n\n"
        "function onUpdate(dt: number)\n"
        "    -- Called every frame\n"
        "end\n\n"
        "function onHit(entityId: number, damage: number, attacker: string)\n"
        "    print(\"Entity \" .. tostring(entityId) .. \" hit by \" .. attacker)\n"
        "end\n";

    m_editor.SetText(defaultCode);
}

void ScriptEditorDock::openFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::stringstream ss;
        ss << file.rdbuf();
        m_editor.SetText(ss.str());
        m_currentFilePath = filePath;
        m_statusMessage = "Loaded: " + filePath;
        m_isDirty = false;
    } else {
        m_statusMessage = "Error opening: " + filePath;
    }
}

void ScriptEditorDock::newScript() {
    m_currentFilePath = "de/scripts/untitled.luau";
    m_editor.SetText("local World = require(\"@de/world\")\n\nfunction onUpdate(dt: number)\nend\n");
    m_statusMessage = "New Script";
    m_isDirty = false;
}

void ScriptEditorDock::saveCurrentFile() {
    std::string text = m_editor.GetText();
    std::string compileError;
    std::string bytecode = ScriptEngine::compileCode(text, &compileError);

    if (bytecode.empty()) {
        m_statusMessage = "Syntax Error: " + compileError;
        return;
    }

    // Save to disk
    std::ofstream file(m_currentFilePath);
    if (file.is_open()) {
        file << text;
        m_statusMessage = "Saved & Compiled cleanly: " + m_currentFilePath;
        m_isDirty = false;

        // Hot reload into ScriptEngine
        ScriptEngine::executeString(text, m_currentFilePath);
    } else {
        m_statusMessage = "Failed to write: " + m_currentFilePath;
    }
}

void ScriptEditorDock::render(bool* p_open) {
    if (!p_open || *p_open) {
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);

        if (ImGui::Begin("Script Editor", p_open, ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New Script")) newScript();
                    if (ImGui::MenuItem("Save", "Ctrl+S")) saveCurrentFile();
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            // Top action bar
            if (ImGui::Button("Save & Run (Ctrl+S)")) {
                saveCurrentFile();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("| %s", m_currentFilePath.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "[%s]", m_statusMessage.c_str());

            // Check Ctrl+S shortcut
            if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S)) {
                saveCurrentFile();
            }

            // Code editor area
            m_editor.Render("##LuauEditorArea");
        }
        ImGui::End();
    }
}

} // namespace Djusov
