#include "Scene3D.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <set>
namespace cygame {
using namespace DirectX;
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void vectorValue(const Json& v,size_t count){need(v.is_array()&&v.size()==count,"Invalid 3D vector size");for(const auto& x:v)need(x.is_number()&&std::isfinite(x.get<double>())&&std::abs(x.get<double>())<=1e8,"Invalid/non-finite 3D vector");}
XMVECTOR vector(const Json& v,float w=0){return XMVectorSet(v[0],v[1],v[2],v.size()==4?v[3].get<float>():w);}
std::array<float,3> xyz(FXMVECTOR v){XMFLOAT3 f;XMStoreFloat3(&f,v);return {f.x,f.y,f.z};}
Json trackValue(const Json& track,float time){
    const auto& times=track.at("times");const auto& values=track.at("values");const std::string mode=track.at("interpolation"),path=track.at("path");
    size_t a=0;while(a+1<times.size()&&times[a+1].get<float>()<=time)++a;size_t b=std::min(a+1,times.size()-1);
    const float span=times[b].get<float>()-times[a].get<float>(),t=span>0?std::clamp((time-times[a].get<float>())/span,0.f,1.f):0;
    if(mode=="STEP"||a==b)return values.at(mode=="CUBICSPLINE"?a*3+1:a);
    Json result=Json::array();const size_t width=path=="rotation"?4:3;
    if(mode=="LINEAR"&&path=="rotation"){
        XMFLOAT4 q;XMStoreFloat4(&q,XMQuaternionNormalize(XMQuaternionSlerp(vector(values[a]),vector(values[b]),t)));return {q.x,q.y,q.z,q.w};
    }
    for(size_t j=0;j<width;++j){float value=0;
        if(mode=="CUBICSPLINE"){const float v0=values[a*3+1][j],v1=values[b*3+1][j],out=values[a*3+2][j],in=values[b*3][j];value=(2*t*t*t-3*t*t+1)*v0+(t*t*t-2*t*t+t)*span*out+(-2*t*t*t+3*t*t)*v1+(t*t*t-t*t)*span*in;}
        else value=values[a][j].get<float>()*(1-t)+values[b][j].get<float>()*t;
        result.push_back(value);
    }
    if(path=="rotation"){XMFLOAT4 q;XMStoreFloat4(&q,XMQuaternionNormalize(vector(result)));result={q.x,q.y,q.z,q.w};}return result;
}
}
void validateScene3D(const Json& model){
    need(model.is_object()&&model.value("format","")=="cygame-scene3d"&&model.value("schemaVersion",0)==1,"Unsupported 3D document");
    need(model.at("assetId").is_string()&&model.at("assetId").get<std::string>().size()==36,"Invalid portable asset identity");
    const auto identity=model.at("assetId").get<std::string>();for(size_t i=0;i<identity.size();++i){bool dash=i==8||i==13||i==18||i==23;need(dash?identity[i]=='-':std::string("0123456789abcdef").find(identity[i])!=std::string::npos,"Portable identity must be a canonical UUID");}
    need(model.at("name").is_string()&&model.at("name").get<std::string>().size()<=512,"Invalid model name");
    need(model.at("source").at("glbBase64").is_string()&&model.at("source").at("glbBase64").get_ref<const std::string&>().size()<2*1024*1024,"Invalid source preservation payload");
    need(model.at("space").value("upAxis","")=="+Y"&&model.at("space").value("handedness","")=="right"&&model.at("space").value("linearUnit","")=="meter"&&model.at("space").value("rotation","")=="quaternion-xyzw"&&model.at("space").value("timeUnit","")=="second","Unsupported native 3D coordinates");
    const auto& nodes=model.at("nodes");const auto& meshes=model.at("meshes");const auto& skins=model.at("skins");
    need(nodes.is_array()&&nodes.size()<=512&&meshes.is_array()&&meshes.size()<=512&&skins.is_array()&&skins.size()<=128,"3D collection limit exceeded");
    for(size_t i=0;i<nodes.size();++i){const auto& n=nodes[i];need(n.at("id")=="node:"+std::to_string(i)&&n.at("name").is_string()&&n.at("name").get<std::string>().size()<=512,"Invalid stable node identity/name");
        vectorValue(n.at("translation"),3);vectorValue(n.at("rotation"),4);vectorValue(n.at("scale"),3);if(n.contains("matrix"))vectorValue(n["matrix"],16);
        need(XMVectorGetX(XMQuaternionLengthSq(vector(n["rotation"])))>1e-10f,"Quaternion cannot be zero");
        need(n.at("parent").is_number_integer()&&n.at("parent").get<double>()>=-1&&n.at("parent").get<double>()<double(nodes.size()),"Invalid node parent index");int parent=n.at("parent");std::set<int> seen={int(i)};while(parent!=-1){need(parent>=0&&parent<int(nodes.size())&&seen.insert(parent).second,"Invalid/cyclic node hierarchy");parent=nodes[parent].at("parent");}
        if(n.contains("mesh"))need(n["mesh"].is_number_integer()&&n["mesh"].get<int>()>=0&&n["mesh"].get<size_t>()<meshes.size(),"Invalid mesh node");
        if(n.contains("skin"))need(n["skin"].is_number_integer()&&n["skin"].get<int>()>=0&&n["skin"].get<size_t>()<skins.size(),"Invalid skin node");
    }
    need(model.at("materials").is_array()&&model["materials"].size()<=512,"Invalid material collection");for(const auto& material:model["materials"]){vectorValue(material.at("baseColor"),4);for(const auto& v:material["baseColor"])need(v.get<double>()>=0&&v.get<double>()<=1,"Material color must be normalized");}
    size_t vertices=0,triangles=0;
    for(size_t mi=0;mi<meshes.size();++mi){const auto& mesh=meshes[mi];need(mesh.at("primitives").is_array()&&mesh["primitives"].size()<=128,"Primitive limit exceeded");
        for(size_t pi=0;pi<mesh["primitives"].size();++pi){const auto& p=mesh["primitives"][pi];const auto& pos=p.at("positions");const auto& indices=p.at("indices");
            need(p.at("material").is_number_integer()&&p.at("material").get<double>()>=-1&&p.at("material").get<double>()<double(model["materials"].size()),"Invalid material slot");need(p.at("sourceMesh")==mi&&p.at("sourcePrimitive")==pi,"Invalid primitive identity");need(pos.is_array()&&!pos.empty()&&indices.is_array()&&!indices.empty()&&indices.size()%3==0,"Invalid triangle geometry");
            vertices+=pos.size();triangles+=indices.size()/3;need(vertices<=20000&&triangles<=20000,"Preview mesh limit is 20,000 vertices/triangles");
            for(const auto& v:pos)vectorValue(v,3);for(const auto& index:indices)need(index.is_number_unsigned()||index.is_number_integer(),"Triangle index is not integral");
            for(const auto& index:indices)need(index.get<int>()>=0&&index.get<size_t>()<pos.size(),"Triangle index outside vertex stream");
            for(const auto& pair:std::vector<std::pair<const char*,size_t>>{{"normals",3},{"uv",2},{"joints",4},{"weights",4}})if(p.contains(pair.first)){need(p[pair.first].size()==pos.size(),"Vertex stream length mismatch");for(const auto& v:p[pair.first])vectorValue(v,pair.second);}
            need(p.contains("joints")==p.contains("weights"),"Joint and weight streams must be paired");
            if(p.contains("weights"))for(size_t vi=0;vi<pos.size();++vi){float sum=0;for(int j=0;j<4;++j){need(p["weights"][vi][j].get<float>()>=0,"Negative skin weight");sum+=p["weights"][vi][j].get<float>();need(p["joints"][vi][j].is_number_integer()&&p["joints"][vi][j].get<int>()>=0&&p["joints"][vi][j].get<int>()<256,"Invalid joint slot");}need(sum>1e-8&&sum<1.001f,"Invalid skin weight total");}
        }
    }
    for(const auto& skin:skins){need(skin.at("joints").is_array()&&!skin["joints"].empty()&&skin["joints"].size()<=256&&skin.at("inverseBindMatrices").size()==skin["joints"].size(),"Invalid skin");std::set<int> unique;for(const auto& joint:skin["joints"]){need(joint.is_number_integer()&&joint.get<double>()>=0&&joint.get<double>()<double(nodes.size()),"Invalid joint index");int j=joint;need(j>=0&&j<int(nodes.size())&&unique.insert(j).second,"Invalid skin joint");}for(const auto& matrix:skin["inverseBindMatrices"])vectorValue(matrix,16);}
    for(const auto& n:nodes)if(n.contains("mesh")&&n.contains("skin")){const auto& skin=skins.at(n["skin"].get<size_t>());for(const auto& p:meshes.at(n["mesh"].get<size_t>())["primitives"]){need(p.contains("joints"),"Skinned primitive has no weights");for(const auto& joints:p["joints"])for(const auto& j:joints)need(j.get<size_t>()<skin["joints"].size(),"Skin slot exceeds skeleton");}}
    size_t keys=0;need(model.at("animations").is_array()&&model["animations"].size()<=128,"Animation limit exceeded");
    for(const auto& clip:model["animations"]){need(clip.at("name").is_string()&&clip["name"].get<std::string>().size()<=512&&clip.at("tracks").is_array()&&clip["tracks"].size()<=1536,"Invalid animation clip");std::set<std::pair<int,std::string>> targets;
        for(const auto& track:clip["tracks"]){int node=track.at("node");const std::string path=track.at("path"),mode=track.at("interpolation");need(node>=0&&node<int(nodes.size())&&!nodes[node].contains("matrix")&&(path=="translation"||path=="rotation"||path=="scale")&&targets.insert({node,path}).second,"Invalid animation target");
            need(mode=="STEP"||mode=="LINEAR"||mode=="CUBICSPLINE","Invalid animation interpolation");const auto& times=track.at("times");const auto& values=track.at("values");need(times.is_array()&&!times.empty()&&values.is_array()&&values.size()==times.size()*(mode=="CUBICSPLINE"?3:1),"Animation key shape mismatch");keys+=times.size();need(keys<=20000,"Animation key limit exceeded");float previous=-1;for(const auto& t:times){need(t.is_number()&&std::isfinite(t.get<float>())&&t.get<float>()>=0&&t.get<float>()>previous&&t.get<float>()<=1e6,"Animation times must increase");previous=t;}for(const auto& v:values)vectorValue(v,path=="rotation"?4:3);if(path=="rotation")for(size_t k=0;k<times.size();++k){const auto& q=values[mode=="CUBICSPLINE"?k*3+1:k];need(XMVectorGetX(XMQuaternionLengthSq(vector(q)))>1e-10,"Animation quaternion cannot be zero");}
        }
    }
    validateModelAuthoring(model);
    need(model.dump().size()<=2*1024*1024,"3D document exceeds 2 MiB");
}
ModelFrame sampleScene3D(const Json& source,int animation,float time,int lod){
    const auto model=modelAtLod(source,lod);
    auto nodes=model.at("nodes");if(animation>=0&&animation<int(model["animations"].size()))for(const auto& track:model["animations"][animation]["tracks"])nodes[track.at("node").get<size_t>()][track.at("path").get<std::string>()]=trackValue(track,time);
    std::vector<XMMATRIX> world(nodes.size(),XMMatrixIdentity());std::vector<bool> done(nodes.size(),false);
    std::function<XMMATRIX(size_t)> transform=[&](size_t i)->XMMATRIX{if(done[i])return world[i];const auto& n=nodes[i];XMMATRIX local;
        if(n.contains("matrix")){XMFLOAT4X4 value;for(int k=0;k<16;++k)(&value._11)[k]=n["matrix"][k];local=XMLoadFloat4x4(&value);}
        else local=XMMatrixScalingFromVector(vector(n["scale"]))*XMMatrixRotationQuaternion(XMQuaternionNormalize(vector(n["rotation"])))*XMMatrixTranslationFromVector(vector(n["translation"]));
        int parent=n.at("parent");world[i]=parent>=0?local*transform(size_t(parent)):local;XMFLOAT4X4 bounded;XMStoreFloat4x4(&bounded,world[i]);for(int k=0;k<16;++k)if(!std::isfinite((&bounded._11)[k])||std::abs((&bounded._11)[k])>1e12f)throw std::runtime_error("Composed 3D transform exceeds preview limits");done[i]=true;return world[i];};
    ModelFrame frame;for(size_t i=0;i<nodes.size();++i){transform(i);frame.nodes.push_back(xyz(XMVector3TransformCoord(XMVectorZero(),world[i])));}
    bool first=true;
    for(size_t ni=0;ni<nodes.size();++ni){const auto& node=nodes[ni];if(!node.contains("mesh"))continue;for(const auto& p:model["meshes"].at(node["mesh"].get<size_t>())["primitives"]){
        std::vector<XMMATRIX> skin;
        if(node.contains("skin")){const auto& s=model["skins"].at(node["skin"].get<size_t>());for(size_t j=0;j<s["joints"].size();++j){XMFLOAT4X4 inverse;for(int k=0;k<16;++k)(&inverse._11)[k]=s["inverseBindMatrices"][j][k];skin.push_back(XMLoadFloat4x4(&inverse)*world[s["joints"][j].get<size_t>()]);}}
        std::vector<std::array<float,3>> positions,normals;positions.reserve(p["positions"].size());
        auto normalMatrix=[](FXMMATRIX matrix){XMVECTOR determinant;auto inverse=XMMatrixInverse(&determinant,matrix);return std::abs(XMVectorGetX(determinant))>1e-12f?XMMatrixTranspose(inverse):XMMatrixIdentity();};
        std::vector<XMMATRIX> skinNormals;for(const auto& matrix:skin)skinNormals.push_back(normalMatrix(matrix));const auto rigidNormal=normalMatrix(world[ni]);
        for(size_t vi=0;vi<p["positions"].size();++vi){XMVECTOR v=vector(p["positions"][vi],1),result=XMVectorZero();
            if(skin.empty())result=XMVector3TransformCoord(v,world[ni]);else{float sum=0;for(int slot=0;slot<4;++slot){float weight=p["weights"][vi][slot];sum+=weight;result+=XMVector3TransformCoord(v,skin[p["joints"][vi][slot].get<size_t>()])*weight;}result/=sum;}
            auto position=xyz(result);for(float component:position)if(!std::isfinite(component)||std::abs(component)>1e12f)throw std::runtime_error("Deformed mesh exceeds preview coordinate limits");positions.push_back(position);
            if(p.contains("normals")){auto n=vector(p["normals"][vi]);XMVECTOR transformed=XMVectorZero();if(skin.empty())transformed=XMVector3TransformNormal(n,rigidNormal);else for(int slot=0;slot<4;++slot)transformed+=XMVector3TransformNormal(n,skinNormals[p["joints"][vi][slot].get<size_t>()])*p["weights"][vi][slot].get<float>();normals.push_back(xyz(XMVector3Normalize(transformed)));}
            for(int j=0;j<3;++j){if(first)frame.minimum[j]=frame.maximum[j]=position[j];else{frame.minimum[j]=std::min(frame.minimum[j],position[j]);frame.maximum[j]=std::max(frame.maximum[j],position[j]);}}first=false;
        }
        std::array<float,3> color{.65f,.68f,.72f};int material=p.value("material",-1);if(material>=0&&material<int(model["materials"].size()))for(int j=0;j<3;++j)color[j]=model["materials"][material]["baseColor"][j];
        for(size_t i=0;i<p["indices"].size();i+=3){const auto a=positions[p["indices"][i].get<size_t>()],b=positions[p["indices"][i+1].get<size_t>()],c=positions[p["indices"][i+2].get<size_t>()];const XMVECTOR av=XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(a.data())),bv=XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(b.data())),cv=XMLoadFloat3(reinterpret_cast<const XMFLOAT3*>(c.data()));auto normal=xyz(XMVector3Normalize(XMVector3Cross(bv-av,cv-av)));
            for(size_t corner=0;corner<3;++corner){const size_t vertex=p["indices"][i+corner];const auto& pos=positions[vertex];const auto& smooth=normals.empty()?normal:normals[vertex];ModelVertex v{};for(int j=0;j<3;++j){v.position[j]=pos[j];v.normal[j]=smooth[j];v.color[j]=color[j];}frame.triangles.push_back(v);}
        }
    }}
    if(first)for(const auto& position:frame.nodes){for(int j=0;j<3;++j){if(first)frame.minimum[j]=frame.maximum[j]=position[j];else{frame.minimum[j]=std::min(frame.minimum[j],position[j]);frame.maximum[j]=std::max(frame.maximum[j],position[j]);}}first=false;}
    return frame;
}
}
