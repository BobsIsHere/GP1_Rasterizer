#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();

		bool SaveBufferToImage() const;

		//------ Render Functions ------
		void Render_W1_Part1();
		void Render_W1_Part2();
		void Render_W1_Part3();
		void Render_W1_Part4();
		void Render_W1_Part5();

		void Render_W2();

		void Render_W3();

		//------ Enum Class ------
		enum RenderMode
		{
			FinalColour,
			DepthBuffer
		};

		//------ Own Functions ------
		float Calculate2DCrossProduct(const Vector3& a, const Vector3& b, const Vector2& c);
		float Remap(float value, float inputMin, float inputMax, float outputMin, float outputMax);

		bool IsPixelInTriangle(const Vector2& p, const std::vector<Vertex>& vertex, const int index);

		void PixelHandeling(int px, int py, int triangleIdx, const std::vector<Vertex>& vertex_transformed);
		void TriangleHandeling(int triangleIdx, const Mesh& mesh_transformed);

		void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const;
		void VertexTransformationFunction(std::vector<Mesh>& meshes_in) const;

		void ClippingTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2);
		void RenderModeCycling();

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		int m_Width{};
		int m_Height{};

		Texture* m_pTexture{ nullptr };

		RenderMode m_RenderMode{};
	};
}
