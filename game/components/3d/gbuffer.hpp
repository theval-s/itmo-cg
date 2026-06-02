#pragma once
#include <d3d11.h>

namespace val_cg {
    class GBuffer {
    public:
        void Initialize(ID3D11Device* device, int width, int height);
        void DestroyResources();
        void Clear(ID3D11DeviceContext* ctx);

        // Bind RT0 + RT1 as render targets for the geometry pass.
        void Bind(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* dsv);

        // Expose RT0+RT1 as SRVs for the lighting pass (startSlot = t0 by default).
        void BindSRVs(ID3D11DeviceContext* ctx, int startSlot = 0);
        void UnbindSRVs(ID3D11DeviceContext* ctx, int startSlot = 0);

        ID3D11ShaderResourceView* srv0 = nullptr;  // DiffuseSpec (t0)
        ID3D11ShaderResourceView* srv1 = nullptr;  // Normal      (t1)

    private:
        ID3D11Texture2D*        tex0 = nullptr;
        ID3D11Texture2D*        tex1 = nullptr;
        ID3D11RenderTargetView* rtv0 = nullptr;
        ID3D11RenderTargetView* rtv1 = nullptr;
    };
}
