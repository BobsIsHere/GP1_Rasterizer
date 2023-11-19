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

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	m_pTexture = Texture::LoadFromFile({ "Resources/uv_grid_2.png" } );
}

Renderer::~Renderer()
{
	delete m_pTexture;
	delete[] m_pDepthBufferPixels;
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
	//Render_W1_Part3();		//barycentric coordinates
	//Render_W1_Part4();		//depth buffer
	//Render_W1_Part5();		//boundingbox optimization
	Render_W2();

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
			const Vector2 p{ px + 0.5f, py + 0.5f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			//check if pixel is inside triangle
			if (Calculate2DCrossProduct(vertices_ndc[2], vertices_ndc[1], p) < 0.f ||
				Calculate2DCrossProduct(vertices_ndc[1], vertices_ndc[0], p) < 0.f ||
				Calculate2DCrossProduct(vertices_ndc[0], vertices_ndc[2], p) < 0.f)
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
			const Vector2 p{ px + 0.5f, py + 0.5f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			//check if pixel is inside triangle
			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, p) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, p) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, p) < 0.f)
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
			const Vector2 p{ px + 0.5f, py + 0.5f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, p) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, p) < 0.f ||
				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, p) < 0.f)
			{
				isIntriangle = false;
			}
			else
			{
				isIntriangle = true;
			}

			if (isIntriangle)
			{
				float w0{ Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[2].position, p) };
				float w1{ Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[0].position, p) };
				float w2{ Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[1].position, p) };

				const float triangleArea{ w0 + w1 + w2 };

				//normalize weights
				w0 /= triangleArea;
				w1 /= triangleArea;
				w2 /= triangleArea;

				finalColour = { vertices_transformed[0].color * w0 +
								vertices_transformed[1].color * w1 +
								vertices_transformed[2].color * w2 };
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
	//define triangle - vertices in WOLRD space
	bool isIntriangle{ false };
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	std::vector<Vertex> vertices_world
	{
		//Triangle 0 
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},
		//Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}},
	};

	std::vector<Vertex> vertices_transformed{};
	vertices_transformed.reserve(vertices_world.size());

	VertexTransformationFunction(vertices_world, vertices_transformed);

	//clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//go over triangle, per 3 vertices
	for (int triangleIdx{}; triangleIdx < vertices_transformed.size(); triangleIdx += 3)
	{
		//go over each pixel is in screen space
		for (int px{}; px < m_Width; ++px)
		{
			for (int py{}; py < m_Height; ++py)
			{
				//define current pixel in screen space
				const Vector2 p{ px + 0.5f, py + 0.5f};
				ColorRGB finalColour{ 0.f, 0.f, 0.f };

				if (Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 1].position, p) < 0.f ||
					Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 0].position, p) < 0.f ||
					Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 2].position, p) < 0.f)
				{
					isIntriangle = false;
				}
				else
				{
					isIntriangle = true;
				}

				if (isIntriangle)
				{
					float w0{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 2].position, p) };
					float w1{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 0].position, p) };
					float w2{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 1].position, p) };
					const float triangleArea{ w0 + w1 + w2 };

					//normalize weights
					w0 /= triangleArea;
					w1 /= triangleArea;
					w2 /= triangleArea;
					
					//interpolate depth value
					const int bufferIdx{ px + (py * m_Width) };
					const float interpolateDepth{ (w0 * vertices_transformed[triangleIdx + 0].position.z + 
												  w1 * vertices_transformed[triangleIdx + 1].position.z +
												  w2 * vertices_transformed[triangleIdx + 2].position.z) };
					
					if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
					{
						m_pDepthBufferPixels[bufferIdx] = interpolateDepth;

						finalColour = { vertices_transformed[triangleIdx + 0].color * w0 +
									    vertices_transformed[triangleIdx + 1].color * w1 +
									    vertices_transformed[triangleIdx + 2].color * w2 };

						finalColour.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColour.r * 255),
							static_cast<uint8_t>(finalColour.g * 255),
							static_cast<uint8_t>(finalColour.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W1_Part5()
{
	//define triangle - vertices in WORLD space
	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	const std::vector<Vertex> vertices_world
	{
		//Triangle 0 
		{{0.f, 2.f, 0.f}, {1, 0, 0}},
		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},
		//Triangle 1
		{{0.f, 4.f, 2.f}, {1, 0, 0}},
		{{3.f, -2.f, 2.f}, {0, 1, 0}},
		{{-3.f, -2.f, 2.f}, {0, 0, 1}},
	};

	std::vector<Vertex> vertices_transformed{};
	VertexTransformationFunction(vertices_world, vertices_transformed);

	//clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//go over triangle, per 3 vertices
	for (int triangleIdx{}; triangleIdx < vertices_transformed.size(); triangleIdx += 3)
	{
		//calculate bounding box for the current triangle in screen space
		const Vector2 v0{ vertices_transformed[triangleIdx].position.x , vertices_transformed[triangleIdx].position.y };
		const Vector2 v1{ vertices_transformed[triangleIdx + 1].position.x,  vertices_transformed[triangleIdx + 1].position.y };
		const Vector2 v2{ vertices_transformed[triangleIdx + 2].position.x, vertices_transformed[triangleIdx + 2].position.y };

		//calculate min & max x of bounding box, clamped to screen
		const int minX{ static_cast<int>(std::max(0.0f, std::min({ v0.x, v1.x, v2.x }))) };
		const int maxX{ static_cast<int>(std::min(static_cast<float>(m_Width - 1), std::max({ v0.x, v1.x, v2.x }))) };
		//calculate min & max y of bounding box, clamped to screen
		const int minY{ static_cast<int>(std::max(0.0f, std::min({ v0.y, v1.y, v2.y }))) };
		const int maxY{ static_cast<int>(std::min(static_cast<float>(m_Height - 1), std::max({ v0.y, v1.y, v2.y }))) };


		//go over each pixel is in screen space
		for (int px{minX}; px < maxX; ++px)
		{
			for (int py{minY}; py < maxY; ++py)
			{
				//define current pixel in screen space
				const Vector2 p{ px + 0.5f, py + 0.5f };
				ColorRGB finalColour{ 0.f, 0.f, 0.f };

				if (IsPixelInTriangle(p,vertices_transformed, triangleIdx))
				{
					float w0{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 2].position, p) };
					float w1{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 0].position, p) };
					float w2{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 1].position, p) };
					//using right formula, see slides, has performance gain too
					const float triangleArea{ w0 + w1 + w2 };

					//normalize weights
					w0 /= triangleArea;
					w1 /= triangleArea;
					w2 /= triangleArea;

					//interpolate depth value
					const int bufferIdx{ px + (py * m_Width) };
					const float interpolateDepth{ w0 * vertices_transformed[triangleIdx + 0].position.z +
												  w1 * vertices_transformed[triangleIdx + 1].position.z +
												  w2 * vertices_transformed[triangleIdx + 2].position.z };


					if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
					{
						m_pDepthBufferPixels[bufferIdx] = interpolateDepth;

						finalColour = { vertices_transformed[triangleIdx + 0].color * (w0 / triangleArea) +
										vertices_transformed[triangleIdx + 1].color * (w1 / triangleArea) +
										vertices_transformed[triangleIdx + 2].color * (w2 / triangleArea) };

						finalColour.MaxToOne();

						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
							static_cast<uint8_t>(finalColour.r * 255),
							static_cast<uint8_t>(finalColour.g * 255),
							static_cast<uint8_t>(finalColour.b * 255));
					}
				}
			}
		}
	}
}

void Renderer::Render_W2()
{
	//define triangle - vertices in WORLD space
	/*std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
			},
			{
				3, 0, 4, 1, 5, 2,
				2, 6,
				6, 3, 7, 4, 8, 5
			},
			PrimitiveTopology::TriangleStrip
		}
	};*/

	std::vector<Mesh> meshes_world
	{
		Mesh
		{
			{
				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
			},
			{
				3, 0, 1,    1, 4, 3,    4, 1, 2,
				2, 5, 4,    6, 3, 4,    4, 7, 6,
				7, 4, 5,    5, 8, 7
			},
			PrimitiveTopology::TriangleList
		}
	};

	std::vector<Mesh> mesh_transformed{};
	mesh_transformed.reserve(meshes_world.size());

	VertexTransformationFunction(meshes_world, mesh_transformed);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	//clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	//extra variable; amount of sides : 0, 1, 2
	const int amountOfSides{ 2 };

	for (Mesh mesh : mesh_transformed)
	{
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			//go over triangle, per 3 vertices
			for (int triangleIdx{}; triangleIdx < mesh.indices.size() - amountOfSides; ++triangleIdx)
			{
				//if it's odd (oneven)
				if (triangleIdx & 1)
				{
					//make triangle clockwise again
					std::swap(mesh.indices[triangleIdx + 1], mesh.indices[triangleIdx + 2]);
					TriangleHandeling(triangleIdx, mesh);
					std::swap(mesh.indices[triangleIdx + 2], mesh.indices[triangleIdx + 1]);
				}
				else
				{
					TriangleHandeling(triangleIdx, mesh);
				}
			}
		}
		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
		{
			//go over triangle, per 3 vertices
			for (int triangleIdx{}; triangleIdx < mesh.indices.size(); triangleIdx += 3)
			{
				TriangleHandeling(triangleIdx, mesh);
			}
		}

		//clear vertices
		mesh_transformed.clear();
	}
}

bool Renderer::IsPixelInTriangle(const Vector2& p, const std::vector<Vertex>& vertex, const int index)
{
	if (Calculate2DCrossProduct(vertex[index + 2].position, vertex[index + 1].position, p) < 0.f)
	{
		return false;
	}
	else if (Calculate2DCrossProduct(vertex[index + 1].position, vertex[index + 0].position, p) < 0.f)
	{
		return false;
	}
	else if (Calculate2DCrossProduct(vertex[index + 0].position, vertex[index + 2].position, p) < 0.f)
	{
		return false;
	}

	return true;
}

void Renderer::PixelHandeling(int px, int py, int triangleIdx, const std::vector<Vertex>& vertex_transformed)
{
	//define current pixel in screen space
	const Vector2 p{ px + 0.5f, py + 0.5f };
	ColorRGB finalColour{ 0.f, 0.f, 0.f };

	if (IsPixelInTriangle(p, vertex_transformed, triangleIdx))
	{
		float w0{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 1].position, vertex_transformed[triangleIdx + 2].position, p) };
		float w1{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 2].position, vertex_transformed[triangleIdx + 0].position, p) };
		float w2{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 0].position, vertex_transformed[triangleIdx + 1].position, p) };
		//using right formula, see slides, has performance gain too
		const float triangleArea{ w0 + w1 + w2 };

		//normalize weights
		w0 /= triangleArea;
		w1 /= triangleArea;
		w2 /= triangleArea;

		//interpolate depth value
		const int bufferIdx{ px + (py * m_Width) };
		const float interpolateDepth{ (w0 * vertex_transformed[triangleIdx + 0].position.z +
									   w1 * vertex_transformed[triangleIdx + 1].position.z +
									   w2 * vertex_transformed[triangleIdx + 2].position.z) };


		if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
		{
			m_pDepthBufferPixels[bufferIdx] = interpolateDepth;

			finalColour = { vertex_transformed[triangleIdx + 0].color * w0 +
							vertex_transformed[triangleIdx + 1].color * w1 +
							vertex_transformed[triangleIdx + 2].color * w2 };

			finalColour.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColour.r * 255),
				static_cast<uint8_t>(finalColour.g * 255),
				static_cast<uint8_t>(finalColour.b * 255));
		}
	}
}

void Renderer::TriangleHandeling(int triangleIdx, const Mesh& mesh_transformed)
{
	//calculate bounding box for the current triangle in screen space
	const Vertex v0{ mesh_transformed.vertices[mesh_transformed.indices[triangleIdx]] };
	const Vertex v1{ mesh_transformed.vertices[mesh_transformed.indices[triangleIdx + 1]] };
	const Vertex v2{ mesh_transformed.vertices[mesh_transformed.indices[triangleIdx + 2]] };

	//calculate min & max x of bounding box, clamped to screen
	const int minX{ static_cast<int>(std::max(0.0f, std::min({ v0.position.x, v1.position.x, v2.position.x }))) };
	const int maxX{ static_cast<int>(std::min(static_cast<float>(m_Width), std::max({ v0.position.x, v1.position.x, v2.position.x }))) };
	//calculate min & max y of bounding box, clamped to screen
	const int minY{ static_cast<int>(std::max(0.0f, std::min({ v0.position.y, v1.position.y, v2.position.y }))) };
	const int maxY{ static_cast<int>(std::min(static_cast<float>(m_Height), std::max({ v0.position.y, v1.position.y, v2.position.y }))) };
	
	//go over each pixel is in screen space
	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
		{
			//define current pixel in screen space
			const Vector2 p{ px + 0.5f, py + 0.5f };
			ColorRGB finalColour{ 0.f, 0.f, 0.f };

			float w0{ Vector2::Cross(v2.position.GetXY() - v1.position.GetXY(), p - v1.position.GetXY()) };
			float w1{ Vector2::Cross(v0.position.GetXY() - v2.position.GetXY(), p - v2.position.GetXY()) };
			float w2{ Vector2::Cross(v1.position.GetXY() - v0.position.GetXY(), p - v0.position.GetXY()) };

			// Something like this is needed
			if (w0 >= 0.f && w1 >= 0.f && w2 >= 0.f)
			{
				//using right formula, see slides, has performance gain too
				const float triangleArea{ w0 + w1 + w2 };
				const float invTriangleArea{ 1.f / triangleArea };

				//normalize weights
				w0 *= invTriangleArea;
				w1 *= invTriangleArea;
				w2 *= invTriangleArea;

				//interpolate depth value
				const int bufferIdx{ px + (py * m_Width) };
				/*const float zInterpolated{ v0.position.z * w0 + 
											  v1.position.z * w1 + 
											  v2.position.z * w2 };*/
				const float invVerticeZ0{ (1.f / v0.position.z) * w0 };
				const float invVerticeZ1{ (1.f / v1.position.z) * w1 };
				const float invVerticeZ2{ (1.f / v2.position.z) * w2 };
				
				const float zInterpolated{ 1.f / (invVerticeZ0 + invVerticeZ1 + invVerticeZ2) };
				
				if (zInterpolated <= m_pDepthBufferPixels[bufferIdx])
				{
					m_pDepthBufferPixels[bufferIdx] = zInterpolated;
					//finalColour = { v0.color * w0 + v1.color * w1 + v2.color * w2 };
					//Vector2 interpolatedUVDepth{ v0.uv * w0 + v1.uv * w1 + v2.uv * w2 };
					Vector2 invUV0{ (v0.uv / v0.position.z) * w0 };
					Vector2 invUV1{ (v1.uv / v1.position.z) * w1 };
					Vector2 invUV2{ (v2.uv / v2.position.z) * w2 };

					Vector2 interpolatedUVDepth{ (invUV0 + invUV1 + invUV2) * zInterpolated };

					finalColour = m_pTexture->Sample(interpolatedUVDepth);

					finalColour.MaxToOne();

					m_pBackBufferPixels[bufferIdx] = SDL_MapRGB(m_pBackBuffer->format,
						static_cast<uint8_t>(finalColour.r * 255),
						static_cast<uint8_t>(finalColour.g * 255),
						static_cast<uint8_t>(finalColour.b * 255));
				}
			}
		}
	}
}

float Renderer::Calculate2DCrossProduct(const Vector3& a, const Vector3& b, const Vector2& c)
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

void Renderer::VertexTransformationFunction(const std::vector<Mesh>& meshes_in, std::vector<Mesh>& meshes_out) const
{
	meshes_out = meshes_in;
	
	for (Mesh& mesh : meshes_out)
	{
		for (Vertex& vertice : mesh.vertices)
		{
			vertice.position = m_Camera.viewMatrix.TransformPoint(vertice.position);

			vertice.position.x = (vertice.position.x / vertice.position.z) / (float(m_Width) / float(m_Height) * m_Camera.fov);
			vertice.position.y = (vertice.position.y / vertice.position.z) / m_Camera.fov;
			vertice.position.z = vertice.position.z;

			vertice.position.x = ((vertice.position.x + 1.f) / 2.f) * m_Width;
			vertice.position.y = ((1.f - vertice.position.y) / 2.f) * m_Height;
		}
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
