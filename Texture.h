﻿#pragma once
#include "Bindable.h"

class Texture : public Bindable
{
public:
	Texture(const Graphics& InGraphics, const class Surface& InSurface, unsigned int InSlot = 0);
	void Bind(const Graphics& InGraphics) noexcept override;

protected:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> MyTextureView;

private:
	unsigned int Slot;
};
