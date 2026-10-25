#include "Editor.h"
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <array>
namespace cygame {
namespace {
struct V {float x,y,z;V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}V operator*(float s)const{return{x*s,y*s,z*s};}};
float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V normalized(V a){return a*(1/std::max(0.0001f,std::sqrt(dot(a,a))));}
V asV(const Json& a){return{a[0].get<float>(),a[1].get<float>(),a[2].get<float>()};}
V rotated(V v,V r) {
    r=r*(3.14159265f/180.f);float c=std::cos(r.x),s=std::sin(r.x);v={v.x,v.y*c-v.z*s,v.y*s+v.z*c};
    c=std::cos(r.y);s=std::sin(r.y);v={v.x*c+v.z*s,v.y,-v.x*s+v.z*c};
    c=std::cos(r.z);s=std::sin(r.z);return{v.x*c-v.y*s,v.x*s+v.y*c,v.z};
}
bool contains(const std::array<ImVec2,4>& p,ImVec2 q) {
    bool inside=false;for(int i=0,j=3;i<4;j=i++)if(((p[i].y>q.y)!=(p[j].y>q.y))&&(q.x<(p[j].x-p[i].x)*(q.y-p[i].y)/(p[j].y-p[i].y)+p[i].x))inside=!inside;
    return inside;
}
}
void EditorApp::viewport(const std::string& id) {
    auto& camera=cameras_[id];const auto& pal=cyui::palette();
    const char* modes[]={tr("Perspective"),tr("Top"),tr("Front")};
    ImGui::SetNextItemWidth(cyui::dp(130));ImGui::Combo("##projection",&camera.mode,modes,3);
    ImGui::SameLine();const char* shading[]={tr("Solid"),tr("Wireframe")};ImGui::SetNextItemWidth(cyui::dp(100));ImGui::Combo("##shading",&camera.drawMode,shading,2);
    ImGui::SameLine();ImGui::Checkbox(tr("Grid"),&camera.grid);
    ImGui::SameLine();
    if(cyui::ghostButton(tr("Focus selection"))&&!core.selection().empty())core.focusEntity(core.selection());
    ImGui::SameLine();cyui::helpMarker(tr("RMB orbit | MMB pan | Wheel zoom | F focus"));
    const char* tools[]={tr("Q Select"),tr("W Move"),tr("E Rotate"),tr("R Scale")};int selectedTool=gizmoOperation_+1;
    if(cyui::segmented("##transformTools",tools,4,&selectedTool,cyui::dp(285)))gizmoOperation_=selectedTool-1;
    ImGui::SameLine();ImGui::Checkbox(tr("Local"),&localTransform_);
    ImGui::SameLine();ImGui::Checkbox(tr("Snap"),&snapping_);
    ImGui::SameLine();ImGui::SetNextItemWidth(cyui::dp(68));float* snap=gizmoOperation_==1?&rotationSnap_:gizmoOperation_==2?&scaleSnap_:&translationSnap_;
    ImGui::DragFloat("##snap",snap,.05f,.01f,100.f,"%.2f",ImGuiSliderFlags_AlwaysClamp);
    if(core.selectedIds().size()>1){ImGui::SameLine();cyui::textFaint("%s",tr("Group: translation"));}
    ImVec2 start=ImGui::GetCursorScreenPos(),size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,1.f);size.y=std::max(size.y,1.f);
    // Reserve the canvas without claiming an active ImGui item: ImGuizmo
    // deliberately refuses activation while any item is hovered or active.
    ImGui::Dummy(size);
    bool hover=ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows)&&ImGui::IsMouseHoveringRect(start,{start.x+size.x,start.y+size.y});auto& io=ImGui::GetIO();
    if(hover&&!io.WantTextInput&&!ImGui::IsMouseDown(ImGuiMouseButton_Right)){
        if(ImGui::IsKeyPressed(ImGuiKey_Q))gizmoOperation_=-1;
        if(ImGui::IsKeyPressed(ImGuiKey_W))gizmoOperation_=0;
        if(ImGui::IsKeyPressed(ImGuiKey_E))gizmoOperation_=1;
        if(ImGui::IsKeyPressed(ImGuiKey_R))gizmoOperation_=2;
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_D))invoke("scene_duplicate");
        if(ImGui::IsKeyPressed(ImGuiKey_Delete))invoke("scene_delete_many");
    }
    if(hover) {
        camera.distance=std::clamp(camera.distance*std::pow(0.88f,io.MouseWheel),0.5f,200.f);
        if(ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            camera.yaw-=io.MouseDelta.x*0.008f;camera.pitch=std::clamp(camera.pitch+io.MouseDelta.y*0.008f,-1.5f,1.5f);
        }
        if(ImGui::IsKeyPressed(ImGuiKey_F)&&!core.selection().empty())core.focusEntity(core.selection());
    }
    if(viewportRenderer){ViewportContext context{start,size,camera,core};viewportRenderer(context);return;}
    if(core.adapter().descriptor().at("id")!="sandbox"){
        cyui::emptyState(tr("Viewport"),tr("The adapter must register a viewport renderer."),cyui::icon::info);return;
    }
    V target{camera.target[0],camera.target[1],camera.target[2]};
    V eye=target+V{std::cos(camera.yaw)*std::cos(camera.pitch),std::sin(camera.yaw)*std::cos(camera.pitch),std::sin(camera.pitch)}*camera.distance;
    V forward=normalized(target-eye),right=normalized(cross(forward,{0,0,1})),up=cross(right,forward);
    if(camera.mode==1){eye=target+V{0,0,camera.distance};forward={0,0,-1};right={1,0,0};up={0,1,0};}
    if(camera.mode==2){eye=target+V{0,-camera.distance,0};forward={0,1,0};right={1,0,0};up={0,0,1};}
    if(hover&&ImGui::IsMouseDragging(ImGuiMouseButton_Middle)){
        target=target+right*(-io.MouseDelta.x*camera.distance/size.y)+up*(io.MouseDelta.y*camera.distance/size.y);
        camera.target[0]=target.x;camera.target[1]=target.y;camera.target[2]=target.z;
    }
    auto project=[&](V v,ImVec2& screen,float& depth) {
        v=v-eye;depth=dot(v,forward);if(depth<0.05f)return false;
        float factor=std::min(size.x,size.y)*0.95f/(camera.mode==0?depth:camera.distance);
        screen={start.x+size.x/2+dot(v,right)*factor,start.y+size.y/2-dot(v,up)*factor};return true;
    };
    if(viewportSelectionProbe&&!core.selection().empty()){ImVec2 point;float depth;if(project(asV(core.entity(core.selection()).at("properties").at("position")),point,depth))viewportSelectionProbe(id,point);}
    auto* draw=ImGui::GetWindowDrawList();ImVec2 end{start.x+size.x,start.y+size.y};
    draw->PushClipRect(start,end,true);draw->AddRectFilled(start,end,pal.bg0);
    auto line=[&](V a,V b,ImU32 color,float width=1.f) {ImVec2 x,y;float d1,d2;if(project(a,x,d1)&&project(b,y,d2))draw->AddLine(x,y,color,cyui::dp(width));};
    if(camera.grid)for(int i=-20;i<=20;++i){line({float(i),-20,0},{float(i),20,0},i%5==0?pal.gridStrong:pal.grid);line({-20,float(i),0},{20,float(i),0},i%5==0?pal.gridStrong:pal.grid);}
    line({0,0,0},{3,0,0},pal.warning,1.5f);line({0,0,0},{0,3,0},pal.success,1.5f);line({0,0,0},{0,0,3},pal.info,1.5f);
    struct Face{std::array<ImVec2,4> p;float depth;ImU32 color;std::string id;bool selected;};
    std::vector<Face> faces;
    const int corners[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
    const float light[6]={0.6f,1.f,0.72f,0.86f,0.68f,0.82f};
    for(const auto& e:core.entities()) {
        auto p=e.at("properties");
        const auto& draft=gizmos_[id];if(draft.active&&draft.revision==core.revision())for(const auto& t:draft.transforms)if(t["entityId"]==e["id"])for(const auto* key:{"position","rotation","scale"})p[key]=t.at(key);
        if(!p.at("enabled").get<bool>())continue;
        const V pos=asV(p.at("position")),scale=asV(p.at("scale")),rotation=asV(p.at("rotation"));
        std::array<V,8> vertices{};
        float matrix[16],trn[]={pos.x,pos.y,pos.z},rot[]={rotation.x,rotation.y,rotation.z},scl[]={scale.x,scale.y,scale.z};
        ImGuizmo::RecomposeMatrixFromComponents(trn,rot,scl,matrix);
        for(int i=0;i<8;++i){float x=i&1?.5f:-.5f,y=i&2?.5f:-.5f,z=i&4?.5f:-.5f;vertices[i]={x*matrix[0]+y*matrix[4]+z*matrix[8]+matrix[12],x*matrix[1]+y*matrix[5]+z*matrix[9]+matrix[13],x*matrix[2]+y*matrix[6]+z*matrix[10]+matrix[14]};}
        const auto tint=p.at("tint");const ImVec4 color{tint[0],tint[1],tint[2],1};
        for(int f=0;f<6;++f){
            Face face{};face.id=e.at("id");face.selected=core.isSelected(face.id);bool valid=true;
            for(int j=0;j<4;++j){float depth=0;valid&=project(vertices[corners[f][j]],face.p[j],depth);face.depth+=depth/4.f;}
            face.color=ImGui::ColorConvertFloat4ToU32({color.x*light[f],color.y*light[f],color.z*light[f],1});
            if(valid)faces.push_back(face);
        }
    }
    std::sort(faces.begin(),faces.end(),[](const Face& a,const Face& b){return a.depth>b.depth;});
    std::string picked;
    for(const auto& f:faces) {
        if(camera.drawMode==0)draw->AddConvexPolyFilled(f.p.data(),4,f.color);
        draw->AddPolyline(f.p.data(),4,f.selected?pal.accent:pal.lineStrong,ImDrawFlags_Closed,cyui::dp(f.selected?1.8f:1.f));
        if(hover&&contains(f.p,io.MousePos))picked=f.id;
    }
    float view[16]={right.x,up.x,-forward.x,0,right.y,up.y,-forward.y,0,right.z,up.z,-forward.z,0,-dot(eye,right),-dot(eye,up),dot(eye,forward),1};
    float projection[16]{};const float nearZ=.05f,farZ=1000.f,extent=std::min(size.x,size.y)*1.9f;
    projection[0]=extent/size.x;projection[5]=extent/size.y;
    if(camera.mode==0){projection[10]=-(farZ+nearZ)/(farZ-nearZ);projection[11]=-1;projection[14]=-2*farZ*nearZ/(farZ-nearZ);}
    else{projection[0]/=camera.distance;projection[5]/=camera.distance;projection[10]=-2/(farZ-nearZ);projection[14]=-(farZ+nearZ)/(farZ-nearZ);projection[15]=1;}
    const bool gizmoOver=transformGizmo(id,start,size,view,projection);
    if(hover&&ImGui::IsMouseClicked(ImGuiMouseButton_Left)&&!gizmoOver&&!picked.empty())invoke("scene_select",{{"entityId",picked},{"mode",io.KeyCtrl?"toggle":"replace"}});
    draw->AddText({start.x+cyui::dp(14),end.y-cyui::dp(26)},pal.textFaint,tr("Z up  /  units: metres  /  preview renderer"));
    draw->PopClipRect();
}
}
