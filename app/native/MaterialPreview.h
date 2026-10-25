#pragma once
#include "Core.h"
#include <imgui.h>
#include <d3d11.h>
#include <wrl/client.h>
namespace cygame {
class MaterialPreview {
public:
    MaterialPreview(ID3D11Device* device,ID3D11DeviceContext* context);
    void install(const std::string& key,const Json& report);
    ImTextureID render(const std::string& key,const Json& material,ImVec2 size);
private:
    template<class T>using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Target{Ptr<ID3D11Texture2D> texture;Ptr<ID3D11RenderTargetView> rtv;Ptr<ID3D11ShaderResourceView> srv;UINT width=0,height=0;};
    Ptr<ID3D11Device> device_;Ptr<ID3D11DeviceContext> context_;
    Ptr<ID3D11VertexShader> vertex_;Ptr<ID3D11PixelShader> pixel_;Ptr<ID3D11Buffer> constants_;Ptr<ID3D11RasterizerState> raster_;Ptr<ID3D11SamplerState> sampler_;
    std::map<std::string,Target> targets_;
    struct Shader{std::string signature;Ptr<ID3D11PixelShader> pixel;std::vector<Ptr<ID3D11ShaderResourceView>> textures;};std::map<std::string,Shader> shaders_;
};
}
