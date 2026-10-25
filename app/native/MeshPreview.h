#pragma once
#include "Scene3D.h"
#include <imgui.h>
#include <d3d11.h>
#include <wrl/client.h>
namespace cygame {
struct ModelCamera {float yaw=.7f,pitch=.35f,distance=2.8f;bool wireframe=false;};
std::array<float,16> modelCameraMatrix(const ModelFrame& frame,const ModelCamera& camera,ImVec2 size);
class MeshPreview {
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    Ptr<ID3D11Device> device_;Ptr<ID3D11DeviceContext> context_;
    Ptr<ID3D11VertexShader> vertex_;Ptr<ID3D11PixelShader> pixel_;Ptr<ID3D11InputLayout> layout_;
    Ptr<ID3D11Buffer> constants_,vertices_;Ptr<ID3D11RasterizerState> solid_,wire_;
    struct Target{UINT w=0,h=0;Ptr<ID3D11Texture2D> texture,depth;Ptr<ID3D11RenderTargetView> rtv;Ptr<ID3D11DepthStencilView> dsv;Ptr<ID3D11ShaderResourceView> srv;};
    std::map<std::string,Target> targets_;size_t capacity_=0;
public:
    MeshPreview(ID3D11Device* device,ID3D11DeviceContext* context);
    void prune(const Json& models){for(auto it=targets_.begin();it!=targets_.end();)if(!models.contains(it->first))it=targets_.erase(it);else ++it;}
    ImTextureID render(const std::string& key,const ModelFrame& frame,const ModelCamera& camera,ImVec2 size);
};
}
