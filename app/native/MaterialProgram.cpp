#include "Graph.h"
#include "Image.h"
#include <fstream>
#include <set>
#include <windows.h>
#include <bcrypt.h>
#include <sstream>
#include <iomanip>
namespace cygame {
std::string sha256(const std::string& bytes){
    BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::string result;std::vector<unsigned char> object,digest(32);
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("SHA-256 unavailable");
    try{
        DWORD size=0,read=0;if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&read,0)<0)throw std::runtime_error("SHA-256 configuration failed");
        object.resize(size);
        if(BCryptCreateHash(algorithm,&hash,object.data(),size,nullptr,0,0)<0||BCryptHashData(hash,reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())),static_cast<ULONG>(bytes.size()),0)<0||BCryptFinishHash(hash,digest.data(),32,0)<0)throw std::runtime_error("SHA-256 failed");
        std::ostringstream text;for(auto b:digest)text<<std::hex<<std::setw(2)<<std::setfill('0')<<int(b);result=text.str();
    }catch(...){if(hash)BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);throw;}
    BCryptDestroyHash(hash);BCryptCloseAlgorithmProvider(algorithm,0);return result;
}
namespace {
Json program(Json graph){graph.erase("assetPath");graph.erase("assetMissing");graph.erase("references");graph.erase("bookmarks");graph.erase("comments");for(auto& n:graph["nodes"]){n.erase("position");n.erase("comment");n.erase("breakpoint");}if(graph.contains("functions"))for(auto& f:graph["functions"])f=program(f);return graph;}
}
Json prepareMaterial(const fs::path& root,const Json& graphs,const std::string& graphId){
    auto resolved=resolveMaterial(root,graphs,graphId,false);auto graph=resolved.at("graph");if(graph.at("domain")!="material")throw std::runtime_error("Preview compilation requires a material");
    const auto diagnostics=graphDiagnostics(graph);if(!diagnostics.at("valid").get<bool>())throw std::runtime_error("Material graph has validation errors");
    auto normalized=program(graph);Json parameters=Json::array(),names=Json::array();
    const auto variables=graph.value("variables",Json::object());for(const auto& [name,v]:variables.items()){
        normalized["variables"][name].erase("default");Json vector=Json::array({0.,0.,0.,0.});const auto value=v.at("default");
        if(value.is_boolean())vector[0]=value.get<bool>()?1.:0.;else if(value.is_number())vector[0]=value;else if(value.is_array())for(size_t i=0;i<3;++i)vector[i]=value[i];
        names.push_back(name);parameters.push_back(vector);
    }
    std::set<std::string> paths;
    std::function<void(const Json&)> scan=[&](const Json& body){for(const auto& node:body.at("nodes"))if(node.at("type")=="material.texture"){
        const std::string path=node.at("values").at("path");validateAssetReferences({{"references",Json::array({path})}});paths.insert(path);
        for(const auto& link:body.at("links"))if(link.at("toNode")==node.at("id")&&link.at("toPin")=="path")throw std::runtime_error("Preview texture paths must be literal asset properties");
    }if(body.contains("functions"))for(const auto& f:body["functions"])scan(f);};scan(graph);if(paths.size()>8)throw std::runtime_error("A material supports up to eight textures");
    Json textures=Json::array();size_t totalBytes=0;for(const auto& path:paths){auto image=readImage(root,path,false);totalBytes+=image.info.at("decodedBytes").get<size_t>();if(totalBytes>64*1024*1024)throw std::runtime_error("Material texture budget is 64 MiB decoded");const auto file=fs::weakly_canonical(root/"data/assets"/fs::u8path(path));std::ifstream input(file,std::ios::binary);std::string bytes(std::istreambuf_iterator<char>(input),{});textures.push_back({{"assetPath",path},{"sourcePath",file.u8string()},{"sha256",sha256(bytes)}});}
    auto textureSignature=textures;for(auto& entry:textureSignature)entry.erase("sourcePath");normalized["_textureBindings"]=textureSignature;
    Json preview={{"baseColor",{.2,.45,.8}},{"roughness",.5},{"metallic",0.}};std::string sampleError;try{preview=sampleMaterial(graph,.5,.5,textureSampler(root));}catch(const std::exception& e){sampleError=e.what();}
    Json result={{"graphId",graphId},{"signature",sha256("cy-preview-dx11-v1/"+normalized.dump())},{"graph",graph},{"textures",textures},{"parameters",parameters},{"parameterNames",names},{"preview",preview}};if(!sampleError.empty())result["sampleError"]=sampleError;return result;
}
}
