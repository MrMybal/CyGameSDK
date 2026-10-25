#pragma once
#include "Image.h"
#include <imgui.h>
#include <d3d11.h>
#include <wrl/client.h>
namespace cygame {
class TexturePreview {
public:
    TexturePreview(ID3D11Device* device,fs::path root):device_(device),root_(std::move(root)){}
    ImTextureID render(const std::string& path,int channel,bool refresh);
private:
    struct Entry{ImageData image;Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> view;int channel=-1;uint64_t use=0;};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;fs::path root_;std::map<std::string,Entry> entries_;uint64_t counter_=0;
};
}
