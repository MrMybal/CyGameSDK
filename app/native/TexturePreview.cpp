#include "TexturePreview.h"
#include <algorithm>
namespace cygame {
ImTextureID TexturePreview::render(const std::string& path,int channel,bool refresh){
    if(channel<0||channel>5)throw std::runtime_error("Unknown image channel");if(refresh)entries_.erase(path);
    if(!entries_.count(path)){
        if(entries_.size()>=8){auto oldest=std::min_element(entries_.begin(),entries_.end(),[](const auto& a,const auto& b){return a.second.use<b.second.use;});entries_.erase(oldest);}
        Entry entry;entry.image=readImage(root_,path,true);entries_.emplace(path,std::move(entry));
    }
    auto& entry=entries_.at(path);entry.use=++counter_;
    if(!entry.view||entry.channel!=channel){
        auto pixels=entry.image.rgba;
        for(size_t i=0;i<pixels.size();i+=4){if(channel==1)pixels[i+3]=255;else if(channel>=2){const auto v=pixels[i+channel-2];pixels[i]=pixels[i+1]=pixels[i+2]=v;pixels[i+3]=255;}}
        D3D11_TEXTURE2D_DESC desc{};desc.Width=entry.image.info.at("width");desc.Height=entry.image.info.at("height");desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{pixels.data(),desc.Width*4,0};Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        if(FAILED(device_->CreateTexture2D(&desc,&data,texture.GetAddressOf())))throw std::runtime_error("Cannot upload image preview");
        entry.view.Reset();if(FAILED(device_->CreateShaderResourceView(texture.Get(),nullptr,entry.view.GetAddressOf())))throw std::runtime_error("Cannot create image preview view");entry.channel=channel;
    }
    return reinterpret_cast<ImTextureID>(entry.view.Get());
}
}
