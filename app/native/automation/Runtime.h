#pragma once
#include <cymcp/mcp.hpp>
#include <cymcp/changes.hpp>
#include <cymcp/performance.hpp>
#include <filesystem>
#include <memory>
#include <mutex>
namespace cygame::automation {
namespace fs=std::filesystem;using cymcp::Json;using cymcp::CallContext;
Json readJson(const fs::path&);void writeJson(const fs::path&,const Json&);
class Runtime {
    fs::path root_,install_,jobStore_;
    bool permitChanges_=false;mutable std::atomic_bool configuredChanges_{false};
    std::timed_mutex pipe_;
    std::string desktop_;
    Json config_,manifest_,workerConfig_=Json::object();
    std::unique_ptr<cymcp::Executor> executor_=std::make_unique<cymcp::Executor>(2);
    std::mutex jobsMutex_;
    std::map<std::string,cymcp::Cancellation> activeJobs_;
    std::shared_ptr<cymcp::ToolGateway> gateway_;
    std::string lastRevision_;
    Json runJob(const std::string&,const Json&,const CallContext&);
    void registerControls();
    bool permitted(const std::string&)const;
public:
    std::shared_ptr<cymcp::Registry> operations;
    std::shared_ptr<cymcp::ResourceRegistry> resources;
    std::shared_ptr<cymcp::ChangeFeed> changes;
    std::shared_ptr<cymcp::PerformanceMonitor> performance;
    Runtime(fs::path root,fs::path install,bool permitChanges=false,fs::path store={},fs::path manifest={});
    ~Runtime();
    Json submit(const std::string&,const Json&,const CallContext&);
    void workerConfig(Json config){workerConfig_=std::move(config);}
    const fs::path& root()const{return root_;}const fs::path& install()const{return install_;}
    const Json& config()const{return config_;}
    bool configuredChanges()const{return configuredChanges_.load();}
    void configuredChanges(bool enabled){configuredChanges_.store(enabled);}
    const Json& manifest()const{return manifest_;}
    Json editor(const std::string&,const Json&,const CallContext& = {});
    std::shared_ptr<const cymcp::Registry> tools(bool compact=true)const;
    cymcp::McpServerOptions serverOptions()const;
    void poll();
    void desktop(const fs::path& run){desktop_=cymcp::path_utf8(run);}
    Json jobs(const CallContext&)const;
    Json jobPage(const CallContext&,size_t offset=0,size_t limit=100,const std::string& state={},const std::string& operation={})const;
    Json job(const std::string&,const CallContext&)const;
    Json cancelJob(const std::string&,const CallContext&);
};
}
