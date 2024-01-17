﻿#pragma once
#include <d3d11.h>
#include <DirectXMath.h>
#include "wrl/client.h"
#include "DXGIInfoManager.h"

class Graphics
{
	friend class Bindable;

public:
	Graphics(HWND InWindowHandle);
	Graphics(const Graphics&) = delete;
	Graphics(Graphics&&) = delete;
	Graphics& operator=(const Graphics&) = delete;
	Graphics& operator=(Graphics&&) = delete;
	~Graphics() = default;

	void EndFrame();
	void ClearBuffer(float InRed, float InGreen, float InBlue) const noexcept;
	void DrawIndexed(UINT InCount) const;

	void SetProjection(DirectX::FXMMATRIX& InProjection)
	{
		Projection = InProjection;
	}

	[[nodiscard]] DirectX::XMMATRIX GetProjection() const
	{
		return Projection;
	}

private:
	DXGIInfoManager InfoManager;
	DirectX::XMMATRIX Projection;
	Microsoft::WRL::ComPtr<ID3D11Device> Device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> DepthStencilView;
};
