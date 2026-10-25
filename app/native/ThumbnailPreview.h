#pragma once
#include "Image.h"
#include <imgui.h>
#include <d3d11.h>
#include <wrl/client.h>
namespace cygame {
class ThumbnailPreview {
public:
    ThumbnailPreview(ID3D11Device* device,fs::path root):device_(device),root_(std::move(root)){}
    ImTextureID render(const std::string& path);
    void clear();
private:
    using View=Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>;
    struct Entry{View view;uint64_t use=0;};
    Microsoft::WRL::ComPtr<ID3D11Device> device_;fs::path root_;std::map<std::string,Entry> entries_;
    std::vector<View> retired_;int frame_=-1,decodedFrame_=-1;uint64_t counter_=0;
};
}
