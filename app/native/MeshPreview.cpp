#include "MeshPreview.h"
#include <cyui/CyUI.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace cygame {
using namespace DirectX;
namespace {void ok(HRESULT hr){if(FAILED(hr))throw std::runtime_error("DX11 mesh preview resource failure");}}
std::array<float,16> modelCameraMatrix(const ModelFrame& frame,const ModelCamera& camera,ImVec2 size){
    XMVECTOR center=XMVectorSet((frame.minimum[0]+frame.maximum[0])*.5f,(frame.minimum[1]+frame.maximum[1])*.5f,(frame.minimum[2]+frame.maximum[2])*.5f,0);
    float radius=.1f;for(int i=0;i<3;++i)radius=std::max(radius,frame.maximum[i]-frame.minimum[i]);
    XMVECTOR direction=XMVectorSet(std::sin(camera.yaw)*std::cos(camera.pitch),std::sin(camera.pitch),std::cos(camera.yaw)*std::cos(camera.pitch),0);
    const auto matrix=XMMatrixLookAtRH(center+direction*radius*camera.distance,center,XMVectorSet(0,1,0,0))*XMMatrixPerspectiveFovRH(.75f,std::max(.1f,size.x/size.y),radius*.001f,radius*100.f);
    XMFLOAT4X4 value;XMStoreFloat4x4(&value,matrix);std::array<float,16> result{};std::memcpy(result.data(),&value,64);return result;
}
MeshPreview::MeshPreview(ID3D11Device* device,ID3D11DeviceContext* context):device_(device),context_(context){
    const char* source=R"(cbuffer Camera:register(b0){row_major float4x4 mvp;} struct V{float3 p:POSITION;float3 n:NORMAL;float3 c:COLOR;};struct O{float4 p:SV_Position;float3 n:NORMAL;float3 c:COLOR;};O vs(V v){O o;o.p=mul(float4(v.p,1),mvp);o.n=v.n;o.c=v.c;return o;}float4 ps(O v):SV_Target{float l=.22+.78*abs(dot(normalize(v.n),normalize(float3(-.4,.8,1))));return float4(v.c*l,1);})";
    Ptr<ID3DBlob> vs,ps,error;ok(D3DCompile(source,std::strlen(source),"CyGameSDK_MeshPreview",nullptr,nullptr,"vs","vs_5_0",0,0,vs.GetAddressOf(),error.GetAddressOf()));error.Reset();ok(D3DCompile(source,std::strlen(source),"CyGameSDK_MeshPreview",nullptr,nullptr,"ps","ps_5_0",0,0,ps.GetAddressOf(),error.GetAddressOf()));
    ok(device_->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,vertex_.GetAddressOf()));ok(device_->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,pixel_.GetAddressOf()));
    D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}};
    ok(device_->CreateInputLayout(elements,3,vs->GetBufferPointer(),vs->GetBufferSize(),layout_.GetAddressOf()));D3D11_BUFFER_DESC cb{};cb.ByteWidth=64;cb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;ok(device_->CreateBuffer(&cb,nullptr,constants_.GetAddressOf()));
    D3D11_RASTERIZER_DESC raster{};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;ok(device_->CreateRasterizerState(&raster,solid_.GetAddressOf()));raster.FillMode=D3D11_FILL_WIREFRAME;ok(device_->CreateRasterizerState(&raster,wire_.GetAddressOf()));
}
ImTextureID MeshPreview::render(const std::string& key,const ModelFrame& frame,const ModelCamera& camera,ImVec2 size){
    auto& target=targets_[key];UINT w=UINT(std::clamp(size.x,32.f,1536.f)),h=UINT(std::clamp(size.y,32.f,1536.f));
    if(w!=target.w||h!=target.h){target={};target.w=w;target.h=h;D3D11_TEXTURE2D_DESC d{};d.Width=w;d.Height=h;d.ArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
        ok(device_->CreateTexture2D(&d,nullptr,target.texture.GetAddressOf()));ok(device_->CreateRenderTargetView(target.texture.Get(),nullptr,target.rtv.GetAddressOf()));ok(device_->CreateShaderResourceView(target.texture.Get(),nullptr,target.srv.GetAddressOf()));d.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;d.BindFlags=D3D11_BIND_DEPTH_STENCIL;ok(device_->CreateTexture2D(&d,nullptr,target.depth.GetAddressOf()));ok(device_->CreateDepthStencilView(target.depth.Get(),nullptr,target.dsv.GetAddressOf()));}
    if(frame.triangles.size()>capacity_){vertices_.Reset();capacity_=frame.triangles.size();D3D11_BUFFER_DESC b{};b.ByteWidth=UINT(capacity_*sizeof(ModelVertex));b.Usage=D3D11_USAGE_DYNAMIC;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;b.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;ok(device_->CreateBuffer(&b,nullptr,vertices_.GetAddressOf()));}
    if(!frame.triangles.empty()){D3D11_MAPPED_SUBRESOURCE m{};ok(context_->Map(vertices_.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&m));std::memcpy(m.pData,frame.triangles.data(),frame.triangles.size()*sizeof(ModelVertex));context_->Unmap(vertices_.Get(),0);}
    Ptr<ID3D11RenderTargetView> oldRT;Ptr<ID3D11DepthStencilView> oldDS;context_->OMGetRenderTargets(1,oldRT.GetAddressOf(),oldDS.GetAddressOf());D3D11_VIEWPORT oldVP{};UINT count=1;context_->RSGetViewports(&count,&oldVP);
    Ptr<ID3D11ShaderResourceView> oldSRV;context_->PSGetShaderResources(0,1,oldSRV.GetAddressOf());ID3D11ShaderResourceView* empty=nullptr;context_->PSSetShaderResources(0,1,&empty);
    auto bg=cyui::vec(cyui::palette().bg0);float clear[]={bg.x,bg.y,bg.z,1};auto* rtv=target.rtv.Get();context_->OMSetRenderTargets(1,&rtv,target.dsv.Get());context_->ClearRenderTargetView(rtv,clear);context_->ClearDepthStencilView(target.dsv.Get(),D3D11_CLEAR_DEPTH,1,0);D3D11_VIEWPORT vp{0,0,float(w),float(h),0,1};context_->RSSetViewports(1,&vp);
    context_->OMSetDepthStencilState(nullptr,0);context_->OMSetBlendState(nullptr,nullptr,0xffffffff);context_->RSSetState(camera.wireframe?wire_.Get():solid_.Get());context_->IASetInputLayout(layout_.Get());context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride=sizeof(ModelVertex),offset=0;auto* vb=vertices_.Get();context_->IASetVertexBuffers(0,1,&vb,&stride,&offset);auto matrix=modelCameraMatrix(frame,camera,{float(w),float(h)});context_->UpdateSubresource(constants_.Get(),0,nullptr,matrix.data(),0,0);auto* cb=constants_.Get();context_->VSSetConstantBuffers(0,1,&cb);context_->VSSetShader(vertex_.Get(),nullptr,0);context_->PSSetShader(pixel_.Get(),nullptr,0);if(!frame.triangles.empty())context_->Draw(UINT(frame.triangles.size()),0);
    // The ImGui DX11 backend fully binds its own shaders/IA/raster/depth state after
    // panel submission. Restore targets/viewport/SRV immediately for other previews.
    auto* previous=oldRT.Get();context_->OMSetRenderTargets(1,&previous,oldDS.Get());if(count)context_->RSSetViewports(1,&oldVP);auto* srv=oldSRV.Get();context_->PSSetShaderResources(0,1,&srv);
    return reinterpret_cast<ImTextureID>(target.srv.Get());
}
}
