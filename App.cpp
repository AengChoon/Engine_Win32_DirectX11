﻿#include "App.h"
#include <random>

#include "Ball.h"
#include "Box.h"
#include "EngineMath.h"

App::App()
	: MyWindow(800, 600, WindowClass::GetName())
{
	class Factory
	{
	public:
		Factory(Graphics& InGraphics)
			: MyGraphics(InGraphics)
		{}

		std::unique_ptr<Drawable> operator()()
		{
			switch (Type(RandomGenerator))
			{
			case 0:
				return std::make_unique<Box>(MyGraphics, RandomGenerator, A, B, C, D, E);
			case 1:
				return std::make_unique<Ball>(MyGraphics, RandomGenerator, A, B, C, D, Longitude, Latitude);
			default: return {};
			}
		}

	private:
		Graphics& MyGraphics;
		std::mt19937 RandomGenerator {std::random_device{}()};
		std::uniform_real_distribution<float> A {0.0f, Math::PI * 2.0f};
		std::uniform_real_distribution<float> B {0.0f, Math::PI * 0.5f};
		std::uniform_real_distribution<float> C {0.0f, Math::PI * 0.08f};
		std::uniform_real_distribution<float> D {6.0f, 20.0f};
		std::uniform_real_distribution<float> E {0.4f, 3.0f};
		std::uniform_int_distribution<int> Latitude {5, 20};
		std::uniform_int_distribution<int> Longitude {10, 40};
		std::uniform_int_distribution<int> Type {0, 1};
	};

	const Factory MyFactory {MyWindow.GetGraphics()};
	Drawables.reserve(DrawablesNum);
	std::generate_n(std::back_inserter(Drawables), DrawablesNum, MyFactory);
	MyWindow.GetGraphics().SetProjection(DirectX::XMMatrixPerspectiveLH(1.0f, 3.0f / 4.0f, 0.5f, 40.0f));
}

int App::Run()
{
	while (true)
	{
		if (const auto ExitCode = Window::ProcessMessages())
		{
			return ExitCode.value();
		}

		DoFrame();
	}
}

void App::DoFrame()
{
	auto DeltaTime = MyTimer.Mark();
	MyWindow.GetGraphics().ClearBuffer(0.07f, 0.0f, 0.12f);

	for (const auto& Drawable : Drawables)
	{
		Drawable->Update(DeltaTime);
		Drawable->Draw(MyWindow.GetGraphics());
	}

	MyWindow.GetGraphics().EndFrame();
}
