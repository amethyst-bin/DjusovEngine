#pragma once

#include "world/World.hpp"
#include "player/Player.hpp"
#include "ui/ScriptEditorDock.hpp"
#include <string>
#include <memory>

struct GLFWwindow;

namespace Djusov {

class EditorUI {
public:
    EditorUI();

    void init(GLFWwindow* window, World* world, Player* player);
    void render(bool& isPlayMode, float& gridSnap, bool& requestBuild);

    uint32_t getSelectedEntityId() const { return m_selectedEntityId; }
    void setSelectedEntityId(uint32_t id) { m_selectedEntityId = id; }

    bool isScriptEditorOpen() const { return m_showScriptEditor; }
    void openScriptInEditor(const std::string& scriptPath);

    bool isSettingsOpen() const { return m_showSettings; }
    void setSettingsOpen(bool open) { m_showSettings = open; }

    bool isBuildDialogOpen() const { return m_showBuildDialog; }
    void setBuildDialogOpen(bool open) { m_showBuildDialog = open; }

    // Build dialog settings getters
    std::string getBuildGameName() const { return m_buildGameName; }
    bool isBuildMultiplayer() const { return m_buildMultiplayer; }
    bool isBuildWindows() const { return m_buildWindows; }
    float getBuildMinFOV() const { return m_buildMinFOV; }
    float getBuildMaxFOV() const { return m_buildMaxFOV; }
    bool getBuildAllowQuickSave() const { return m_buildAllowQuickSave; }

private:
    void renderMenuBar(bool& isPlayMode, bool& requestBuild);
    void renderToolbar(bool& isPlayMode, float& gridSnap);
    void renderSceneHierarchy();
    void renderInspector();
    void renderPalette(float gridSnap);
    void renderSettings();
    void renderBuildDialog(bool& requestBuild);

    GLFWwindow* m_window;
    World* m_world;
    Player* m_player;

    uint32_t m_selectedEntityId;

    bool m_showScriptEditor;
    bool m_showSettings;
    bool m_showBuildDialog;
    bool m_showDemoWindow;

    ScriptEditorDock m_scriptEditorDock;

    // Build configuration
    char m_buildGameName[64];
    bool m_buildMultiplayer;
    bool m_buildWindows;
    float m_buildMinFOV;
    float m_buildMaxFOV;
    bool m_buildAllowQuickSave;
};

} // namespace Djusov
