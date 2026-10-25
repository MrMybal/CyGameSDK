
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
// CY_MATERIAL
float4 ps(Vertex input):SV_Target{
    float2 q=input.uv*2-1;q.x*=parameters.z;
    float r2=dot(q,q);if(options.x<.5&&r2>.76)return float4(background.rgb,1);
    float3 n=options.x>.5?float3(0,0,1):normalize(float3(q.x,-q.y,sqrt(max(.76-r2,0)))),v=float3(0,0,1);
    float2 uv=options.x>.5?input.uv:float2(atan2(n.x,n.z)/6.28318530718+.5,acos(clamp(n.y,-1,1))/3.14159265359);
    float3 albedo;float rough,metal;cy_material(uv,albedo,rough,metal);rough=clamp(rough,.035,1);metal=saturate(metal);
    float3 color=light(n,v,normalize(float3(-.7,.8,1.2)),float3(4,3.9,3.7),albedo,rough,metal);
    color+=light(n,v,normalize(float3(.9,.1,.4)),float3(.5,.65,.9),albedo,rough,metal);
    color+=albedo*(.035+.06*saturate(n.y*.5+.5))*(1-metal)+lerp(float3(.04,.04,.04),albedo,metal)*(.03+.2*pow(1-max(n.z,0),3));
    color=1-exp(-color*parameters.w);return float4(pow(saturate(color),1/2.2),1);
}
