#include "ThumbnailPreview.h"
#include <algorithm>
namespace cygame {
void ThumbnailPreview::clear(){for(auto& [key,entry]:entries_)if(entry.view)retired_.push_back(entry.view);entries_.clear();}
ImTextureID ThumbnailPreview::render(const std::string& path){
    auto frame=ImGui::GetFrameCount();if(frame_!=frame){retired_.clear();frame_=frame;}
    auto found=entries_.find(path);if(found!=entries_.end()){found->second.use=++counter_;return reinterpret_cast<ImTextureID>(found->second.view.Get());}
    if(decodedFrame_==frame)return {};decodedFrame_=frame;
    if(entries_.size()>=64){auto oldest=std::min_element(entries_.begin(),entries_.end(),[](const auto& a,const auto& b){return a.second.use<b.second.use;});if(oldest->second.view)retired_.push_back(oldest->second.view);entries_.erase(oldest);}
    Entry entry;entry.use=++counter_;
    try{
        auto image=readImage(root_,path,true);const unsigned sw=image.info.at("width"),sh=image.info.at("height");
        const unsigned width=96,height=48;const float ratio=std::min(96.f/sw,48.f/sh);
        const unsigned dw=std::max(1u,unsigned(sw*ratio)),dh=std::max(1u,unsigned(sh*ratio)),ox=(width-dw)/2,oy=(height-dh)/2;
        std::vector<unsigned char> pixels(size_t(width)*height*4,0);
        for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){const auto i=(size_t(y+oy)*width+x+ox)*4,j=(size_t(y*sh/dh)*sw+x*sw/dw)*4;std::copy_n(image.rgba.data()+j,4,pixels.data()+i);}
        D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{pixels.data(),width*4,0};Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        if(FAILED(device_->CreateTexture2D(&desc,&data,texture.GetAddressOf()))||FAILED(device_->CreateShaderResourceView(texture.Get(),nullptr,entry.view.GetAddressOf())))entry.view.Reset();
    }catch(const std::exception&){} // Unsupported/damaged files retain the asset type icon until refresh.
    auto texture=entry.view.Get();entries_.emplace(path,std::move(entry));return reinterpret_cast<ImTextureID>(texture);
}
}
