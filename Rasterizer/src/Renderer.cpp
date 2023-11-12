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
	//Initialize Camera
	m_Camera.Initialize(60.f, { 0.f, 0.f, -10.f });

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
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

	//Render_W1_Part1();		//rasterizer stage only
	//Render_W1_Part2();		//projection stage (camera)
	Render_W1_Part3();		//barycentric coordinates
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
	//variables
	bool isInTraingle{ false };
	//define triangle - vertices in WORLD space
	std::vector<Vertex> vertices_world
	{
		{ {0.f, 2.f, 0.f} },
		{ {1.f, 0.f, 0.f} },
		{ {-1.f, 0.f, 0.f} }
	};

	std::vector<Vertex> vertices_transformed{};
	vertices_transformed.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_transformed);

	//go over each pixel is in screen space
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define current pixel in screen space
			const Vector3 P{ px + 0.5f, py + 0.5f, 0.f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			//check if pixel is inside triangle
			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, P) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, P) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, P) < 0.f)
			{
				isInTraingle = false;
			}
			else
			{
				isInTraingle = true;
			}

			if (isInTraingle)
			{
				finalColour = { 1.f, 1.f, 1.f };
			}

			finalColour.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColour.r * 255),
				static_cast<uint8_t>(finalColour.g * 255),
				static_cast<uint8_t>(finalColour.b * 255));
		}
	}
}

void Renderer::Render_W1_Part3()
{
	//define triangle - vertices in WOLRD space
	bool isIntriangle{ false };
	std::vector<Vertex> vertices_world
	{
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
	};

	std::vector<Vertex> vertices_transformed{};
	vertices_transformed.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_transformed);

	//go over each pixel is in screen space
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//define current pixel in screen space
			const Vector3 P{ px + 0.5f, py + 0.5f, 0.f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, P) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, P) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, P) < 0.f)
			{
				isIntriangle = false;
			}
			else
			{
				isIntriangle = true;
			}

			if (isIntriangle)
			{
				const float w0{ Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[2].position, P) };
				const float w1{ Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[0].position, P) };
				const float w2{ Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[1].position, P) };
				const float triangleArea{ Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[1].position, vertices_transformed[2].position) };

				finalColour = { vertices_transformed[0].color * (w0 / triangleArea) +
								vertices_transformed[1].color * (w1 / triangleArea) +
								vertices_transformed[2].color * (w2 / triangleArea) };
			}

			finalColour.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColour.r * 255),
				static_cast<uint8_t>(finalColour.g * 255),
				static_cast<uint8_t>(finalColour.b * 255));
		}
	}
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

float Renderer::Calculate2DCrossProduct(const Vector2& a, const Vector2& b, const Vector2& c)
{
	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{	
	for (int idx{}; idx < vertices_in.size(); ++idx)
	{
		const Vector3 worldToView{ m_Camera.viewMatrix.TransformPoint(vertices_in[idx].position) };
		Vector3 projectedVertex{};

		projectedVertex.x = (worldToView.x / worldToView.z) / (float(m_Width) / float(m_Height) * m_Camera.fov);
		projectedVertex.y = (worldToView.y / worldToView.z) / m_Camera.fov;
		projectedVertex.z = worldToView.z;

		projectedVertex.x = ((projectedVertex.x + 1) / 2) * m_Width;
		projectedVertex.y = ((1 - projectedVertex.y) / 2) * m_Height;

		vertices_out.emplace_back(Vertex{ projectedVertex, vertices_in[idx].color });
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
