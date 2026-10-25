#include "MaterialPreview.h"
#include "Graph.h"
#include "Image.h"
#include <fstream>
#include <cyui/CyUI.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cstring>
namespace cygame {
namespace {
void check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
const char* fallbackSource=R"hlsl(
cbuffer Material : register(b0){float4 baseColor;float4 parameters;float4 background;float4 options;float4 cy_params[256];}
struct Vertex{float4 position:SV_Position;float2 uv:TEXCOORD0;};
Vertex vs(uint id:SV_VertexID){
    float2 p=id==0?float2(-1,-1):id==1?float2(-1,3):float2(3,-1);
    Vertex o;o.position=float4(p,0,1);o.uv=float2((p.x+1)*.5,(1-p.y)*.5);return o;
}
float3 light(float3 n,float3 v,float3 l,float3 radiance,float3 albedo,float rough,float metal){
    float3 h=normalize(v+l);float nv=max(dot(n,v),.001),nl=max(dot(n,l),0),nh=max(dot(n,h),0),vh=max(dot(v,h),0);
    float a=rough*rough,a2=a*a,den=nh*nh*(a2-1)+1;
    float d=a2/max(3.14159265*den*den,.00001);
    float k=(rough+1)*(rough+1)/8;
    float visibility=nv/(nv*(1-k)+k)*nl/max(nl*(1-k)+k,.0001);
    float3 f0=lerp(float3(.04,.04,.04),albedo,metal);
    float3 f=f0+(1-f0)*pow(1-vh,5);
    return ((1-f)*(1-metal)*albedo/3.14159265+d*visibility*f/(4*nv*max(nl,.001)))*radiance*nl;
}
float4 ps(Vertex input):SV_Target{
    float2 q=input.uv*2-1;q.x*=parameters.z;
    float r2=dot(q,q);if(options.x<.5&&r2>.76)return float4(background.rgb,1);
    float3 n=options.x>.5?float3(0,0,1):normalize(float3(q.x,-q.y,sqrt(max(.76-r2,0)))),v=float3(0,0,1);
    float rough=clamp(parameters.x,.035,1),metal=saturate(parameters.y);float3 albedo=saturate(baseColor.rgb);
    float3 color=light(n,v,normalize(float3(-.7,.8,1.2)),float3(4,3.9,3.7),albedo,rough,metal);
    color+=light(n,v,normalize(float3(.9,.1,.4)),float3(.5,.65,.9),albedo,rough,metal);
    color+=albedo*(.035+.06*saturate(n.y*.5+.5))*(1-metal)+lerp(float3(.04,.04,.04),albedo,metal)*(.03+.2*pow(1-max(n.z,0),3));
    color=1-exp(-color*parameters.w);return float4(pow(saturate(color),1/2.2),1);
}
)hlsl";
}
MaterialPreview::MaterialPreview(ID3D11Device* device,ID3D11DeviceContext* context):device_(device),context_(context){
    Microsoft::WRL::ComPtr<ID3DBlob> vertex,pixel,error;
    auto compile=[&](const char* entry,const char* profile,ID3DBlob** result){
        auto status=D3DCompile(fallbackSource,std::strlen(fallbackSource),"CyGameSDK_MaterialPreview",nullptr,nullptr,entry,profile,D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,result,error.ReleaseAndGetAddressOf());
        if(FAILED(status))throw std::runtime_error(error?std::string(static_cast<const char*>(error->GetBufferPointer()),error->GetBufferSize()):"Material preview shader compilation failed");
    };
    compile("vs","vs_5_0",vertex.GetAddressOf());compile("ps","ps_5_0",pixel.GetAddressOf());
    check(device_->CreateVertexShader(vertex->GetBufferPointer(),vertex->GetBufferSize(),nullptr,vertex_.GetAddressOf()),"Cannot create preview vertex shader");
    check(device_->CreatePixelShader(pixel->GetBufferPointer(),pixel->GetBufferSize(),nullptr,pixel_.GetAddressOf()),"Cannot create preview pixel shader");
    D3D11_BUFFER_DESC buffer{};buffer.ByteWidth=64+256*16;buffer.Usage=D3D11_USAGE_DEFAULT;buffer.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    check(device_->CreateBuffer(&buffer,nullptr,constants_.GetAddressOf()),"Cannot create preview constant buffer");
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;
    check(device_->CreateRasterizerState(&raster,raster_.GetAddressOf()),"Cannot create preview rasterizer");
    D3D11_SAMPLER_DESC sample{};sample.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sample.AddressU=sample.AddressV=sample.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sample.MaxLOD=D3D11_FLOAT32_MAX;check(device_->CreateSamplerState(&sample,sampler_.GetAddressOf()),"Cannot create preview texture sampler");
}
void MaterialPreview::install(const std::string& key,const Json& report){
    const auto path=fs::u8path(report.at("shaderPath").get<std::string>());if(fs::file_size(path)>1024*1024)throw std::runtime_error("Compiled preview exceeds 1 MiB");
    std::ifstream file(path,std::ios::binary);std::string bytes(std::istreambuf_iterator<char>(file),{});
    if(bytes.size()<4||bytes.substr(0,4)!="DXBC"||sha256(bytes)!=report.at("bytecodeSha256"))throw std::runtime_error("Preview bytecode checksum or format mismatch");
    Shader shader;shader.signature=report.at("signature");check(device_->CreatePixelShader(bytes.data(),bytes.size(),nullptr,shader.pixel.GetAddressOf()),"Cannot create compiled preview shader");
    for(const auto& binding:report.value("textures",Json::array())){auto image=readImageFile(fs::u8path(binding.at("filePath").get<std::string>()),true);D3D11_TEXTURE2D_DESC desc{};desc.Width=image.info.at("width");desc.Height=image.info.at("height");desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA pixels{};pixels.pSysMem=image.rgba.data();pixels.SysMemPitch=desc.Width*4;Ptr<ID3D11Texture2D> texture;Ptr<ID3D11ShaderResourceView> view;check(device_->CreateTexture2D(&desc,&pixels,texture.GetAddressOf()),"Cannot upload preview texture");check(device_->CreateShaderResourceView(texture.Get(),nullptr,view.GetAddressOf()),"Cannot create preview texture view");shader.textures.push_back(view);}
    shaders_[key]=shader;
}
ImTextureID MaterialPreview::render(const std::string& key,const Json& material,ImVec2 size){
    auto& target=targets_[key];UINT width=static_cast<UINT>(std::clamp(size.x,64.f,512.f)),height=static_cast<UINT>(std::clamp(size.y,64.f,512.f));
    if(!target.texture||target.width!=width||target.height!=height){
        target={};D3D11_TEXTURE2D_DESC desc{};desc.Width=width;desc.Height=height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        check(device_->CreateTexture2D(&desc,nullptr,target.texture.GetAddressOf()),"Cannot create material preview texture");
        check(device_->CreateRenderTargetView(target.texture.Get(),nullptr,target.rtv.GetAddressOf()),"Cannot create material preview target");
        check(device_->CreateShaderResourceView(target.texture.Get(),nullptr,target.srv.GetAddressOf()),"Cannot create material preview resource");target.width=width;target.height=height;
    }
    struct Data{float color[4],parameters[4],background[4],options[4],values[256][4];}data{};
    for(int i=0;i<3;++i)data.color[i]=material.at("baseColor")[i];data.color[3]=1;
    data.parameters[0]=material.value("roughness",.5f);data.parameters[1]=material.value("metallic",0.f);data.parameters[2]=float(width)/height;data.parameters[3]=1.2f;
    data.options[0]=material.value("_previewPlane",false)?1.f:0.f;
    if(material.contains("_shaderParameters")){const auto& values=material.at("_shaderParameters");for(size_t i=0;i<std::min(size_t(256),values.size());++i)for(size_t j=0;j<4;++j)data.values[i][j]=values[i][j];}
    const auto background=cyui::vec(cyui::palette().bg0);data.background[0]=background.x;data.background[1]=background.y;data.background[2]=background.z;data.background[3]=1;
    Ptr<ID3D11RenderTargetView> oldRTV;Ptr<ID3D11DepthStencilView> oldDSV;context_->OMGetRenderTargets(1,oldRTV.GetAddressOf(),oldDSV.GetAddressOf());
    D3D11_VIEWPORT oldVP{};UINT count=1;context_->RSGetViewports(&count,&oldVP);
    Ptr<ID3D11VertexShader> oldVS;Ptr<ID3D11PixelShader> oldPS;context_->VSGetShader(oldVS.GetAddressOf(),nullptr,nullptr);context_->PSGetShader(oldPS.GetAddressOf(),nullptr,nullptr);
    Ptr<ID3D11Buffer> oldCB;context_->PSGetConstantBuffers(0,1,oldCB.GetAddressOf());
    Ptr<ID3D11ShaderResourceView> oldSRVs[8];ID3D11ShaderResourceView* oldViews[8]{};context_->PSGetShaderResources(0,8,oldViews);for(int i=0;i<8;++i)oldSRVs[i].Attach(oldViews[i]);Ptr<ID3D11SamplerState> oldSampler;context_->PSGetSamplers(0,1,oldSampler.GetAddressOf());
    Ptr<ID3D11InputLayout> oldLayout;context_->IAGetInputLayout(oldLayout.GetAddressOf());D3D11_PRIMITIVE_TOPOLOGY oldTopology;context_->IAGetPrimitiveTopology(&oldTopology);
    Ptr<ID3D11BlendState> oldBlend;float blendFactor[4];UINT sampleMask;context_->OMGetBlendState(oldBlend.GetAddressOf(),blendFactor,&sampleMask);
    Ptr<ID3D11DepthStencilState> oldDepth;UINT stencil;context_->OMGetDepthStencilState(oldDepth.GetAddressOf(),&stencil);
    Ptr<ID3D11RasterizerState> oldRaster;context_->RSGetState(oldRaster.GetAddressOf());
    ID3D11ShaderResourceView* empty=nullptr;context_->PSSetShaderResources(0,1,&empty);
    auto* rtv=target.rtv.Get();context_->OMSetRenderTargets(1,&rtv,nullptr);D3D11_VIEWPORT vp{0,0,float(width),float(height),0,1};context_->RSSetViewports(1,&vp);
    context_->RSSetState(raster_.Get());context_->OMSetBlendState(nullptr,nullptr,0xffffffff);context_->OMSetDepthStencilState(nullptr,0);
    context_->IASetInputLayout(nullptr);context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context_->VSSetShader(vertex_.Get(),nullptr,0);auto shader=pixel_.Get();auto compiled=shaders_.find(key);if(compiled!=shaders_.end()&&material.value("_shaderSignature","")==compiled->second.signature)shader=compiled->second.pixel.Get();context_->PSSetShader(shader,nullptr,0);ID3D11ShaderResourceView* views[8]{};if(compiled!=shaders_.end()&&shader==compiled->second.pixel.Get())for(size_t i=0;i<compiled->second.textures.size();++i)views[i]=compiled->second.textures[i].Get();context_->PSSetShaderResources(0,8,views);auto* sampler=sampler_.Get();context_->PSSetSamplers(0,1,&sampler);context_->UpdateSubresource(constants_.Get(),0,nullptr,&data,0,0);
    auto* cb=constants_.Get();context_->PSSetConstantBuffers(0,1,&cb);context_->Draw(3,0);
    auto* prevRTV=oldRTV.Get();context_->OMSetRenderTargets(1,&prevRTV,oldDSV.Get());if(count)context_->RSSetViewports(1,&oldVP);
    context_->VSSetShader(oldVS.Get(),nullptr,0);context_->PSSetShader(oldPS.Get(),nullptr,0);auto* prevCB=oldCB.Get();context_->PSSetConstantBuffers(0,1,&prevCB);
    for(int i=0;i<8;++i)oldViews[i]=oldSRVs[i].Get();context_->PSSetShaderResources(0,8,oldViews);auto* prevSampler=oldSampler.Get();context_->PSSetSamplers(0,1,&prevSampler);context_->IASetInputLayout(oldLayout.Get());context_->IASetPrimitiveTopology(oldTopology);
    context_->OMSetBlendState(oldBlend.Get(),blendFactor,sampleMask);context_->OMSetDepthStencilState(oldDepth.Get(),stencil);context_->RSSetState(oldRaster.Get());
    return reinterpret_cast<ImTextureID>(target.srv.Get());
}
}
