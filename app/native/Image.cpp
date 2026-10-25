#include "Image.h"
#include "Graph.h"
#include <cmath>
#include <algorithm>
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
namespace cygame {
ImageData readImage(const fs::path& root,const std::string& relative,bool pixels){
    const auto base=fs::weakly_canonical(root/"data/assets"),path=fs::weakly_canonical(base/fs::u8path(relative)),rel=path.lexically_relative(base);
    if(rel.empty()||rel.is_absolute()||*rel.begin()==".."||!fs::is_regular_file(path))throw std::runtime_error("Image must be an existing file inside the content root");
    auto result=readImageFile(path,pixels);result.info["path"]=rel.generic_u8string();return result;
}
ImageData readImageFile(const fs::path& path,bool pixels){
    if(!fs::is_regular_file(path))throw std::runtime_error("Image file is missing");
    if(fs::file_size(path)>32*1024*1024)throw std::runtime_error("Image file exceeds 32 MiB");
    struct Apartment{HRESULT status=CoInitializeEx(nullptr,COINIT_MULTITHREADED);~Apartment(){if(SUCCEEDED(status))CoUninitialize();}} apartment;
    auto check=[](HRESULT hr,const char* message){if(FAILED(hr))throw std::runtime_error(message);};
    if(FAILED(apartment.status)&&apartment.status!=RPC_E_CHANGED_MODE)throw std::runtime_error("Image decoder COM initialization failed");
    using Microsoft::WRL::ComPtr;ComPtr<IWICImagingFactory> factory;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
    check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(factory.GetAddressOf())),"WIC image decoder unavailable");
    check(factory->CreateDecoderFromFilename(path.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,decoder.GetAddressOf()),"Unsupported or damaged image");
    GUID format{};check(decoder->GetContainerFormat(&format),"Cannot read image format");std::string name;
    if(format==GUID_ContainerFormatPng)name="PNG";else if(format==GUID_ContainerFormatJpeg)name="JPEG";else if(format==GUID_ContainerFormatBmp)name="BMP";else throw std::runtime_error("The image editor supports PNG, JPEG and BMP");
    check(decoder->GetFrame(0,frame.GetAddressOf()),"Cannot read image frame");UINT width=0,height=0;check(frame->GetSize(&width,&height),"Cannot read image dimensions");
    if(!width||!height||width>8192||height>8192||uint64_t(width)*height>4194304)throw std::runtime_error("Image preview exceeds 8192 pixels per side or 4 million pixels");
    ImageData result;result.info={{"path",path.generic_u8string()},{"format",name},{"width",width},{"height",height},{"bytes",fs::file_size(path)},{"decodedBytes",uint64_t(width)*height*4},{"channels","RGBA8"}};
    if(pixels){ComPtr<IWICFormatConverter> converter;check(factory->CreateFormatConverter(converter.GetAddressOf()),"Cannot create RGBA converter");check(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom),"Cannot convert image to RGBA8");result.rgba.resize(size_t(width)*height*4);check(converter->CopyPixels(nullptr,width*4,static_cast<UINT>(result.rgba.size()),result.rgba.data()),"Cannot decode image pixels");}
    return result;
}
TextureSampler textureSampler(const fs::path& root){
    auto images=std::make_shared<std::map<std::string,ImageData>>();
    return [root,images](const std::string& path,double u,double v)->Json{
        if(!std::isfinite(u)||!std::isfinite(v))throw std::runtime_error("UV coordinates must be finite");
        if(!images->count(path)){if(images->size()>=8)throw std::runtime_error("A material supports up to eight textures");auto next=readImage(root,path,true);size_t total=next.rgba.size();for(const auto& image:*images)total+=image.second.rgba.size();if(total>64*1024*1024)throw std::runtime_error("Material texture budget is 64 MiB decoded");(*images)[path]=std::move(next);}
        const auto& image=images->at(path);int w=image.info.at("width"),h=image.info.at("height");double x=(u-std::floor(u))*w-.5,y=(v-std::floor(v))*h-.5;int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=x-ix,fy=y-iy;
        auto channel=[&](int px,int py,int c){px=(px%w+w)%w;py=(py%h+h)%h;double value=image.rgba[(size_t(py)*w+px)*4+c]/255.;return c==3?value:value<=.04045?value/12.92:std::pow((value+.055)/1.055,2.4);};Json rgba=Json::array();
        for(int c=0;c<4;++c)rgba.push_back((channel(ix,iy,c)*(1-fx)+channel(ix+1,iy,c)*fx)*(1-fy)+(channel(ix,iy+1,c)*(1-fx)+channel(ix+1,iy+1,c)*fx)*fy);return rgba;
    };
}

}
