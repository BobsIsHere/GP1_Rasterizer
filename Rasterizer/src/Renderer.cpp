//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"
#include <iostream>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
	delete m_pFrontBuffer;
	delete m_pBackBuffer;
	delete m_pBackBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	Render_W1_Part1();		//rasterizer stage only
	//Render_W1_Part2();		//projection stage (camera)
	//Render_W1_Part3();		//barycentric coordinates
	//Render_W1_Part4();		//depth buffer
	//Render_W1_Part5();		//boundingbox optimization

	//RENDER LOGIC
	//for (int px{}; px < m_Width; ++px)
	//{
	//	for (int py{}; py < m_Height; ++py)
	//	{	
	//		float gradient = px / static_cast<float>(m_Width);
	//		gradient += py / static_cast<float>(m_Width);
	//		gradient /= 2.0f;

	//		ColorRGB finalColor{ gradient, gradient, gradient };

	//		//Update Color in Buffer
	//		finalColor.MaxToOne();

	//		m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
	//			static_cast<uint8_t>(finalColor.r * 255),
	//			static_cast<uint8_t>(finalColor.g * 255),
	//			static_cast<uint8_t>(finalColor.b * 255));
	//	}
	//}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::Render_W1_Part1()
{
	//variables
	bool isInTraingle{ false };
	std::vector<Vector3> vertices_ndc
	{
		{0.f, 0.5f, 1.f},
		{0.5f, -0.5f, 1.f},
		{-0.5f, -0.5f, 1.f},
	};

	//convert from Normilazed Device Coordinates to Screen Space
	for (Vector3& vertice : vertices_ndc)
	{
		vertice.x = ((vertice.x + 1) / 2) * m_Width;
		vertice.y = ((1 - vertice.y) / 2) * m_Height;
	}

	//go over each pixel is in screen space
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define current pixel in screen space
			const Vector3 P{ px + 0.5f, py + 0.5f, 0.f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			//check if pixel is inside triangle
			if (Calculate2DCrossProduct(vertices_ndc[2], vertices_ndc[1], P) < 0.f ||
				Calculate2DCrossProduct(vertices_ndc[1], vertices_ndc[0], P) < 0.f ||
				Calculate2DCrossProduct(vertices_ndc[0], vertices_ndc[2], P) < 0.f)
			{
				isInTraingle = false;
			}
			else
			{
				isInTraingle = true;
			}

			if (isInTraingle)
			{
				finalColour = {1.f, 1.f, 1.f};
			}

			finalColour.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColour.r * 255),
				static_cast<uint8_t>(finalColour.g * 255),
				static_cast<uint8_t>(finalColour.b * 255));
		}
	}
}

void Renderer::Render_W1_Part2()
{

}

void Renderer::Render_W1_Part3()
{

}

void Renderer::Render_W1_Part4()
{

}

void Renderer::Render_W1_Part5()
{

}

float Renderer::Calculate2DCrossProduct(const Vector3& a, const Vector3& b, const Vector3& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
