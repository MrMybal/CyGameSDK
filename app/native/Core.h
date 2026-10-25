#pragma once
#include <json.hpp>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace cygame {
using Json = nlohmann::json;
class GraphExecution;
namespace fs = std::filesystem;
bool matches(const std::string& text, const std::string& query);
Json readJson(const fs::path& path);
void writeJson(const fs::path& path, const Json& value, size_t byteLimit = 8*1024*1024);
void validateValue(const Json& value, const Json& schema, const std::string& path = "parameters");
std::string uniqueId();
void validateEditableJson(const Json& value);

// Engine adapters are independent of the UI and transport.
class IGameAdapter {
public:
    virtual ~IGameAdapter() = default;
    virtual Json descriptor() const = 0;
    virtual Json propertySchema(const Json& entity) const = 0;
    virtual Json createEntity(const std::string& id, const std::string& name) const = 0;
    virtual Json initialScene() const = 0;
    virtual Json validateScene(const Json& entities) const = 0;
    virtual Json assetCodecs() const { return Json::array(); }
    virtual Json buildTargets() const { return Json::array(); }
};
struct ImportDefinition {Json descriptor;std::function<Json(const fs::path&)> inspect;};
struct Panel {
    std::string id, title, owner;
    std::function<void()> draw;
    bool open = true, focus = false;
};
struct Command {
    Json descriptor;
    std::function<Json(const Json&)> run;
    bool undoable = false;
};
class EditorCore {
public:
    explicit EditorCore(fs::path root,std::unique_ptr<IGameAdapter> adapter = nullptr);
    void registerCommand(Command command);
    void registerImporter(ImportDefinition definition);
    void bindCommand(const std::string& id,std::function<Json(const Json&)> handler);
    Json execute(const std::string& id, const Json& parameters = Json::object());
    Json describeCommands() const;
    Json search(const std::string& query) const;
    Json& entity(const std::string& id);
    const Json& entities() const { return document_.at("entities"); }
    const Json& models()const;
    const Json& graphs() const { return document_.at("graphs"); }
    bool graphDirty(const std::string& graphId) const;
    const std::vector<std::string>& selectedIds()const{return selectedIds_;}
    bool isSelected(const std::string& id)const;
    uint64_t revision()const{return revision_;}
    const std::string& selection() const { return selected_; }
    IGameAdapter& adapter() { return *adapter_; }
    const fs::path& root() const { return root_; }
    bool dirty() const { return document_ != saved_; }
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    const std::string& logText() const { return log_; }
    void log(const std::string& message);
    std::vector<Panel> panels;
    std::function<Json(const std::string&, bool)> openPanel;
    std::function<Json(const std::string&)> showReferences,showGraphDiff,showTexture;
    std::function<void(const std::string&)> focusEntity;
    std::function<Json(const std::string&, bool)> setPlugin;
    std::function<void()> resetLayout,requestClose;
    std::function<Json(const std::string&,const Json&)> workspaceAction;
    std::function<Json(const std::string&,const Json&)> buildUi,materialUi,importUi;
    std::function<Json(const Json&)> textureUi;
private:
    fs::path root_;
    std::unique_ptr<IGameAdapter> adapter_;
    std::map<std::string, Command> commands_;
    Json document_, saved_;
    std::map<std::string,Json> savedGraphs_,savedModels_;
    std::map<std::string,std::string> jsonFileHashes_,modelFileHashes_;
    std::map<std::string,std::shared_ptr<GraphExecution>> debugSessions_;
    std::string selected_, log_;
    std::vector<std::string> selectedIds_;
    uint64_t revision_=0;
    struct Snapshot { Json document; std::string selected; std::vector<std::string> ids; std::string label; };
    std::vector<Snapshot> undo_, redo_;
    Json modelAuthoringCommand(const std::string& id,const Json& p);
    Json jsonCommand(const std::string& id,const Json& p);
    Json modelCommand(const std::string& id,const Json& p);
    Json sceneCommand(const std::string& id,const Json& parameters);
    Json buildCommand(const std::string& id,const Json& parameters);
    std::map<std::string,ImportDefinition> importers_;
    Json importCommand(const std::string& id,const Json& parameters);
    Json materialCommand(const std::string& id,const Json& parameters);
    Json assetCommand(const std::string& id,const Json& parameters);
    Json nativeCommand(const std::string& id, const Json& parameters);
};
class IEditorPlugin {
public:
    virtual ~IEditorPlugin() = default;
    virtual std::string id() const = 0;
    virtual void activate(EditorCore& host) = 0;
    virtual void deactivate(EditorCore& host) = 0;
};
}
