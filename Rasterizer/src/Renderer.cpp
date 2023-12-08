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
	const float aspectRatio{ float(m_Width) / float(m_Height) };
	m_Camera.Initialize(aspectRatio, 45.f, { 0.f, 5.f, -64.f });

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//vehicle textures
	m_pDiffuseTexture = Texture::LoadFromFile({ "Resources/vehicle_diffuse.png" } );
	m_pGlossTexture = Texture::LoadFromFile({ "Resources/vehicle_gloss.png" } );
	m_pNormalTexture = Texture::LoadFromFile({ "Resources/vehicle_normal.png" } );
	m_pSpecularTexture = Texture::LoadFromFile({ "Resources/vehicle_specular.png" } );

	//make vehicle mesh
	Mesh mesh{};
	Utils::ParseOBJ("Resources/vehicle.obj", mesh.vertices, mesh.indices);
	m_MeshesObject.emplace_back(mesh);

	//initialize enum variables
	m_RenderMode  = RenderMode::finalColour; 
	m_ShadingMode = ShadingMode::combinedMode; 
}

Renderer::~Renderer()
{
	delete m_pDiffuseTexture;
	delete m_pGlossTexture;
	delete m_pNormalTexture;
	delete m_pSpecularTexture;
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if (m_IsRotating)
	{
		MeshRotation(pTimer); 
	}
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
	//Render_W2();
	//RenderQuad_W3(); 
	//RenderMesh_W3();
	RenderMesh_W4();

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::RenderMesh_W4()
{
	//from world to view to projection to screen space
	VertexTransformationFunction(m_MeshesObject);

	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);

	//clear back buffer
	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));

	for (Mesh mesh : m_MeshesObject)
	{
		if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
		{
			//extra variable; amount of sides : 0, 1, 2
			const auto& maxIdx{ mesh.indices.size() - 2 };

			//go over triangle, per 3 vertices
			for (int triangleIdx{}; triangleIdx < maxIdx; ++triangleIdx)
			{
				TriangleHandeling(triangleIdx, mesh);
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
	}
}

void Renderer::TriangleHandeling(int triangleIdx, const Mesh& mesh_transformed)
{	
	//calculate bounding box for the current triangle in screen space
	const Vertex_Out v0{ mesh_transformed.vertices_out[mesh_transformed.indices[triangleIdx + 0]] };
	Vertex_Out v1{ mesh_transformed.vertices_out[mesh_transformed.indices[triangleIdx + 1]] };
	Vertex_Out v2{ mesh_transformed.vertices_out[mesh_transformed.indices[triangleIdx + 2]] };

	//if it's odd (oneven)
	if (triangleIdx & 1 and mesh_transformed.primitiveTopology == PrimitiveTopology::TriangleStrip)
	{
		//swap variables, make triangle counter-clockwise
		v1 = { mesh_transformed.vertices_out[mesh_transformed.indices[triangleIdx + 2]] };
		v2 = { mesh_transformed.vertices_out[mesh_transformed.indices[triangleIdx + 1]] };
	}

	//frustum culling
	if (v0.position.x < 0 || v0.position.x > m_Width || v0.position.y < 0 || v0.position.y > m_Height || 
		v1.position.x < 0 || v1.position.x > m_Width || v1.position.y < 0 || v1.position.y > m_Height || 
		v2.position.x < 0 || v2.position.x > m_Width || v2.position.y < 0 || v2.position.y > m_Height) 
	{
		return;
	}

	//precompute constants
	const Vector2 v2_v1{ v2.position.GetXY() - v1.position.GetXY() }; 
	const Vector2 v0_v2{ v0.position.GetXY() - v2.position.GetXY() }; 
	const Vector2 v1_v0{ v1.position.GetXY() - v0.position.GetXY() };

	//scale to increase bounding box size -> no lines between triangles/quads
	const float boundingBoxScale{ 5.f }; 

	//calculate min & max x of bounding box, clamped to screen
	const float topLeftX{ std::min({v0.position.x, v1.position.x, v2.position.x}) }; 
	const float topLeftY{ std::min({v0.position.y, v1.position.y, v2.position.y}) }; 
	const int minX{ Clamp(static_cast<int>(topLeftX - boundingBoxScale), 0, m_Width) }; 
	const int minY{ Clamp(static_cast<int>(topLeftY - boundingBoxScale), 0, m_Height) }; 

	//calculate min & max y of bounding box, clamped to screen
	const float bottomRightX{ std::max({v0.position.x, v1.position.x, v2.position.x}) }; 
	const float bottomRightY{ std::max({v0.position.y, v1.position.y, v2.position.y}) }; 
	const int maxX{ Clamp(static_cast<int>(bottomRightX + boundingBoxScale), 0, m_Width) }; 
	const int maxY{ Clamp(static_cast<int>(bottomRightY + boundingBoxScale), 0, m_Height) }; 

	//go over each pixel is in screen space
	for (int px{ minX }; px < maxX; ++px)
	{
		for (int py{ minY }; py < maxY; ++py)
		{
			//define current pixel in screen space
			const Vector2 p{ px + 0.5f, py + 0.5f };

			float w0{ Vector2::Cross(v2_v1, p - v1.position.GetXY()) };
			float w1{ Vector2::Cross(v0_v2, p - v2.position.GetXY()) };
			float w2{ Vector2::Cross(v1_v0, p - v0.position.GetXY()) };

			if (w0 >= 0.f && w1 >= 0.f && w2 >= 0.f)
			{
				ProcessRenderedTriangle(v0, v1, v2, w0, w1, w2, px, py);
			}
		}
	}
}

void Renderer::ProcessRenderedTriangle(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, float w0, float w1, float w2, int px, int py)
{
	//variables
	const int bufferIdx{ px + (py * m_Width) };
	ColorRGB finalColour{ 0.f, 0.f, 0.f };

	//using right formula, see slides, has performance gain too
	const float triangleArea{ w0 + w1 + w2 };
	const float invTriangleArea{ 1.f / triangleArea };

	//normalize weights
	w0 *= invTriangleArea;
	w1 *= invTriangleArea;
	w2 *= invTriangleArea;

	//depth buffer -> only for comparing depth values; are not linear
	const float invVerticeZ0{ (1.f / v0.position.z) * w0 }; 
	const float invVerticeZ1{ (1.f / v1.position.z) * w1 }; 
	const float invVerticeZ2{ (1.f / v2.position.z) * w2 }; 
	//used for comparison in depth test and value we store in depth buffer
	float zBufferValue{ 1.f / (invVerticeZ0 + invVerticeZ1 + invVerticeZ2) };

	//check if value is in range of [0,1]
	if (0.f > zBufferValue || zBufferValue > 1.f)
	{
		return;
	}

	if (zBufferValue <= m_pDepthBufferPixels[bufferIdx])
	{
		m_pDepthBufferPixels[bufferIdx] = zBufferValue;

		//intepolate vertex attributes with correct depth
		const float invVerticeW0{ (1.f / v0.position.w) * w0 };
		const float invVerticeW1{ (1.f / v1.position.w) * w1 };
		const float invVerticeW2{ (1.f / v2.position.w) * w2 };
		float wInterpolated{ 1.f / (invVerticeW0 + invVerticeW1 + invVerticeW2) };

		//calculate interpolated uv coordinates
		const Vector2 invUV0{ (v0.uv / v0.position.w) * w0 };
		const Vector2 invUV1{ (v1.uv / v1.position.w) * w1 };
		const Vector2 invUV2{ (v2.uv / v2.position.w) * w2 };
		Vector2 interpolatedUV{ (invUV0 + invUV1 + invUV2) * wInterpolated };

		//clamp interpolated uv value between [0, 1]
		interpolatedUV.x = Clamp(interpolatedUV.x, 0.f, 1.f);
		interpolatedUV.y = Clamp(interpolatedUV.y, 0.f, 1.f);

		//calculate interpolated colour coordinates
		const ColorRGB invColour0{ (v0.color / v0.position.w) * w0 };  
		const ColorRGB invColour1{ (v1.color / v1.position.w) * w1 }; 
		const ColorRGB invColour2{ (v2.color / v2.position.w) * w2 }; 
		ColorRGB interpolatedColour{ (invColour0 + invColour1 + invColour2) * wInterpolated }; 

		//calculate interpolated normal coordinates
		const Vector3 invNormal0{ (v0.normal / v0.position.w) * w0 }; 
		const Vector3 invNormal1{ (v1.normal / v1.position.w) * w1 }; 
		const Vector3 invNormal2{ (v2.normal / v2.position.w) * w2 }; 
		Vector3 interpolatedNormal{ (invNormal0 + invNormal1 + invNormal2) * wInterpolated }; 

		//calculate interpolated tangent coordinates
		const Vector3 invTangent0{ (v0.tangent / v0.position.w) * w0 }; 
		const Vector3 invTangent1{ (v1.tangent / v1.position.w) * w1 }; 
		const Vector3 invTangent2{ (v2.tangent / v2.position.w) * w2 }; 
		Vector3 interpolatedTangent{ (invTangent0 + invTangent1 + invTangent2) * wInterpolated }; 

		//calculate interpolated viewDirection coordinates
		const Vector3 invViewDirection0{ (v0.viewDirection / v0.position.w) * w0 };
		const Vector3 invViewDirection1{ (v1.viewDirection / v1.position.w) * w1 };
		const Vector3 invViewDirection2{ (v2.viewDirection / v2.position.w) * w2 };
		Vector3 interpolatedViewDirection{ (invViewDirection0 + invViewDirection1 + invViewDirection2) * wInterpolated };

		Vertex_Out vertexOut{};
		vertexOut.uv            = interpolatedUV;
		vertexOut.color         = interpolatedColour; 
		vertexOut.normal        = interpolatedNormal.Normalized(); 
		vertexOut.tangent       = interpolatedTangent.Normalized(); 
		vertexOut.viewDirection = interpolatedViewDirection.Normalized();

		switch (m_RenderMode)
		{
		case Renderer::finalColour:
			finalColour = PixelShading(vertexOut);
			break;
		case Renderer::depthBuffer:
			zBufferValue = Remap(zBufferValue, 0.9975f, 1.f);
			finalColour = ColorRGB{ zBufferValue, zBufferValue, zBufferValue };
			break;
		}

		finalColour.MaxToOne();

		m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
			static_cast<uint8_t>(finalColour.r * 255),
			static_cast<uint8_t>(finalColour.g * 255),
			static_cast<uint8_t>(finalColour.b * 255));
	}
}

float Renderer::Remap(float value, float inputMin, float inputMax)
{
	const float temp{ (value - inputMin) / (inputMax - inputMin) };
	return temp;
}

ColorRGB Renderer::PixelShading(const Vertex_Out& v)
{
	//const variables
	const ColorRGB ambient{ 0.03f, 0.03f , 0.03f }; 
	const Vector3 lightDirection{ 0.577f, -0.577f, 0.577f }; 
	const float lightIntensity{ 7.f };
	const float diffuseCoeffient{ 1.f }; 
	const float shininess{ 25.f };

	//variables
	float observedArea{};
	ColorRGB finalColour{};

	//sample texture maps
	const ColorRGB diffuseColour{ m_pDiffuseTexture->Sample(v.uv) }; 
	const ColorRGB glossColour{ m_pGlossTexture->Sample(v.uv) };
	const ColorRGB normalTextureSample{ m_pNormalTexture->Sample(v.uv) }; 
	const ColorRGB specularColour{ m_pSpecularTexture->Sample(v.uv) };

	//create tangent space transformation matrix
	const Vector3 binormal{ Vector3::Cross(v.normal, v.tangent) };
	const Matrix tangentSpaceAxis{ v.tangent, binormal, v.normal, {0.f, 0.f, 0.f} };

	//sample from normal map and multiply it with matrix
	Vector3 sampledNormal{ normalTextureSample.r, normalTextureSample.g, normalTextureSample.b }; 

	//change range [0, 1] to [-1, 1]
	sampledNormal = 2.f * sampledNormal - Vector3{ 1.f, 1.f, 1.f }; 
	sampledNormal = tangentSpaceAxis.TransformVector(sampledNormal).Normalized();

	if (m_IsShowingNormalMap)
	{
		//observed area
		observedArea = Vector3::Dot(sampledNormal, -lightDirection);
	}
	else
	{
		//observed area
		observedArea = Vector3::Dot(v.normal, -lightDirection);
	}

	if (observedArea <= 0) 
	{
		return ColorRGB{ 0.f, 0.f, 0.f }; 
	}
	
	//shading mode calculations
	const ColorRGB exponent{ glossColour * shininess }; 

	//calculate lambert diffuse
	const ColorRGB lambertDiffuse{ (diffuseCoeffient * diffuseColour) / float(M_PI) }; 

	//calculate phong reflection
	const Vector3 reflect{ lightDirection - (2.f * Vector3::Dot(sampledNormal, lightDirection) * sampledNormal) };
	const float angle{ std::max(0.f, Vector3::Dot(reflect, -v.viewDirection)) };
	const ColorRGB specular{ specularColour * std::powf(angle, exponent.r) };

	switch (m_ShadingMode)
	{
	case Renderer::observedArea:
		finalColour = ColorRGB{ observedArea, observedArea, observedArea };
		break;
	case Renderer::diffuseMode:
		finalColour = lightIntensity * lambertDiffuse * observedArea;
		break;
	case Renderer::specularMode:
		finalColour = specular * observedArea; 
		break;
	case Renderer::combinedMode:
		finalColour = ((lambertDiffuse * lightIntensity) + specular + ambient) * observedArea;
		break;
	}

	return finalColour;
}

void Renderer::VertexTransformationFunction(std::vector<Mesh>& meshes_in) const
{
	for (Mesh& mesh : meshes_in)
	{
		const Matrix worldViewProjectionMatrix{ mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix };
		mesh.vertices_out.clear();
		mesh.vertices_out.reserve(mesh.vertices.size());

		for (Vertex& vertice : mesh.vertices)
		{
			Vector4 transformedPosition{ worldViewProjectionMatrix.TransformPoint(Vector4{vertice.position, 1.f}) };

			//first was overriding vertices normal & tangent
			//making temp variable instead
			const Vector3 newNormal{ mesh.worldMatrix.TransformVector(vertice.normal).Normalized() };
			const Vector3 newTangent{ mesh.worldMatrix.TransformVector(vertice.tangent).Normalized() };
			const Vector3 newViewDirection{ mesh.worldMatrix.TransformVector(vertice.position) - m_Camera.origin };

			//model to NDC space
			transformedPosition.x /= transformedPosition.w;
			transformedPosition.y /= transformedPosition.w;
			transformedPosition.z /= transformedPosition.w;

			//projection to screen space
			transformedPosition.x = ((transformedPosition.x + 1.f) / 2.f) * m_Width;
			transformedPosition.y = ((1.f - transformedPosition.y) / 2.f) * m_Height;

			Vertex_Out& vertex_out{ mesh.vertices_out.emplace_back(Vertex_Out{}) };
			vertex_out.position = transformedPosition;
			vertex_out.color = vertice.color;
			vertex_out.uv = vertice.uv;
			vertex_out.normal = newNormal; 
			vertex_out.tangent = newTangent; 
			vertex_out.viewDirection = newViewDirection; 
		}
	}
}

bool Renderer::ClipAgainstNearFarPlane(const Vertex_Out& v0, const Vertex_Out& v1, const Vertex_Out& v2, const float nearPlane, const float farPlane, std::vector<Vertex_Out>& clippedVertices)
{
	//check if all vertices are behind the near plane or in front of the far plane
	if ((v0.position.z < nearPlane && v1.position.z < nearPlane && v2.position.z < nearPlane) || 
		(v0.position.z > farPlane && v1.position.z > farPlane && v2.position.z > farPlane)) 
	{
		//no clipping needed
		return false;
	}

	//clip against the near plane 
	if (v0.position.z < nearPlane || v1.position.z < nearPlane || v2.position.z < nearPlane)  
	{
		float temp0{ nearPlane / v0.position.z }; 
		float temp1{ nearPlane / v1.position.z }; 
		float temp2{ nearPlane / v2.position.z }; 

		//calculate new clipped vertices and append them to the vector
		clippedVertices.emplace_back(Vector4{ v0.position.x * temp0, v0.position.y * temp0, nearPlane, 1.f });
		clippedVertices.emplace_back(Vector4{ v1.position.x * temp1, v1.position.y * temp1, nearPlane, 1.f });
		clippedVertices.emplace_back(Vector4{ v2.position.x * temp2, v2.position.y * temp2, nearPlane, 1.f });
	}

	//clip against the far plane
	if (v0.position.z > farPlane || v1.position.z > farPlane || v2.position.z > farPlane) 
	{
		float temp0{ farPlane / v0.position.z }; 
		float temp1{ farPlane / v1.position.z }; 
		float temp2{ farPlane / v2.position.z };

		//calculate new clipped vertices and append them to the vector
		clippedVertices.emplace_back(Vector4{ v0.position.x * temp0, v0.position.y * temp0, farPlane, 1.f });
		clippedVertices.emplace_back(Vector4{ v1.position.x * temp1, v1.position.y * temp1, farPlane, 1.f });
		clippedVertices.emplace_back(Vector4{ v2.position.x * temp2, v2.position.y * temp2, farPlane, 1.f });
	}

	//clipping occurred
	return true;
}

void Renderer::SetIsRotating()
{
	m_IsRotating = !m_IsRotating;
}

void Renderer::SetIsShowingNormalMap()
{
	m_IsShowingNormalMap = !m_IsShowingNormalMap;
}

void Renderer::RenderModeCycling()
{
	int temp{ static_cast<int>(m_RenderMode) };
	m_RenderMode = static_cast<RenderMode>((++temp) % 2);
}

void Renderer::ShadingModeCycling()
{
	int temp{ static_cast<int>(m_ShadingMode) };
	m_ShadingMode = static_cast<ShadingMode>((++temp) % 4);
}

void Renderer::MeshRotation(Timer* pTimer)
{
	//update rotation of object
	const Matrix rotation{ Matrix::CreateRotationY((PI_DIV_4 * pTimer->GetElapsed())) };

	for (Mesh& mesh : m_MeshesObject)
	{
		mesh.worldMatrix = rotation * mesh.worldMatrix;
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


//void Renderer::Render_W1_Part1()
//{
//	//variables
//	bool isInTraingle{ false };
//	std::vector<Vector3> vertices_ndc
//	{
//		{0.f, 0.5f, 1.f},
//		{0.5f, -0.5f, 1.f},
//		{-0.5f, -0.5f, 1.f},
//	};
//
//	//convert from Normilazed Device Coordinates to Screen Space
//	for (Vector3& vertice : vertices_ndc)
//	{
//		vertice.x = ((vertice.x + 1) / 2) * m_Width;
//		vertice.y = ((1 - vertice.y) / 2) * m_Height;
//	}
//
//	//go over each pixel is in screen space
//	for (int px{}; px < m_Width; ++px)
//	{
//		for (int py{}; py < m_Height; ++py)
//		{
//			//define current pixel in screen space
//			const Vector2 p{ px + 0.5f, py + 0.5f };
//			ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//			//check if pixel is inside triangle
//			if (Calculate2DCrossProduct(vertices_ndc[2], vertices_ndc[1], p) < 0.f ||
//				Calculate2DCrossProduct(vertices_ndc[1], vertices_ndc[0], p) < 0.f ||
//				Calculate2DCrossProduct(vertices_ndc[0], vertices_ndc[2], p) < 0.f)
//			{
//				isInTraingle = false;
//			}
//			else
//			{
//				isInTraingle = true;
//			}
//
//			if (isInTraingle)
//			{
//				finalColour = {1.f, 1.f, 1.f};
//			}
//
//			finalColour.MaxToOne();
//
//			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//				static_cast<uint8_t>(finalColour.r * 255),
//				static_cast<uint8_t>(finalColour.g * 255),
//				static_cast<uint8_t>(finalColour.b * 255));
//		}
//	}
//}
//
//void Renderer::Render_W1_Part2()
//{
//	//variables
//	bool isInTraingle{ false };
//	//define triangle - vertices in WORLD space
//	std::vector<Vertex> vertices_world
//	{
//		{ {0.f, 2.f, 0.f} },
//		{ {1.f, 0.f, 0.f} },
//		{ {-1.f, 0.f, 0.f} }
//	};
//
//	std::vector<Vertex> vertices_transformed{};
//	vertices_transformed.reserve(vertices_world.size());
//
//	VertexTransformationFunction(vertices_world, vertices_transformed);
//
//	//go over each pixel is in screen space
//	for (int px{}; px < m_Width; ++px)
//	{
//		for (int py{}; py < m_Height; ++py)
//		{
//			//define current pixel in screen space
//			const Vector2 p{ px + 0.5f, py + 0.5f };
//			ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//			//check if pixel is inside triangle
//			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, p) < 0.f ||
//				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, p) < 0.f ||
//				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, p) < 0.f)
//			{
//				isInTraingle = false;
//			}
//			else
//			{
//				isInTraingle = true;
//			}
//
//			if (isInTraingle)
//			{
//				finalColour = { 1.f, 1.f, 1.f };
//			}
//
//			finalColour.MaxToOne();
//
//			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//				static_cast<uint8_t>(finalColour.r * 255),
//				static_cast<uint8_t>(finalColour.g * 255),
//				static_cast<uint8_t>(finalColour.b * 255));
//		}
//	}
//}
//
//void Renderer::Render_W1_Part3()
//{
//	//define triangle - vertices in WOLRD space
//	bool isIntriangle{ false };
//	std::vector<Vertex> vertices_world
//	{
//		{{0.f, 4.f, 2.f}, {1, 0, 0}},
//		{{3.f, -2.f, 2.f}, {0, 1, 0}},
//		{{-3.f, -2.f, 2.f}, {0, 0, 1}}
//	};
//
//	std::vector<Vertex> vertices_transformed{};
//	vertices_transformed.reserve(vertices_world.size());
//
//	VertexTransformationFunction(vertices_world, vertices_transformed);
//
//	//go over each pixel is in screen space
//	for (int px{}; px < m_Width; ++px)
//	{
//		for (int py{}; py < m_Height; ++py)
//		{
//			//define current pixel in screen space
//			const Vector2 p{ px + 0.5f, py + 0.5f };
//			ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//			if (Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[1].position, p) < 0.f ||
//				Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[0].position, p) < 0.f ||
//				Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[2].position, p) < 0.f)
//			{
//				isIntriangle = false;
//			}
//			else
//			{
//				isIntriangle = true;
//			}
//
//			if (isIntriangle)
//			{
//				float w0{ Calculate2DCrossProduct(vertices_transformed[1].position, vertices_transformed[2].position, p) };
//				float w1{ Calculate2DCrossProduct(vertices_transformed[2].position, vertices_transformed[0].position, p) };
//				float w2{ Calculate2DCrossProduct(vertices_transformed[0].position, vertices_transformed[1].position, p) };
//
//				const float triangleArea{ w0 + w1 + w2 };
//
//				//normalize weights
//				w0 /= triangleArea;
//				w1 /= triangleArea;
//				w2 /= triangleArea;
//
//				finalColour = { vertices_transformed[0].color * w0 +
//								vertices_transformed[1].color * w1 +
//								vertices_transformed[2].color * w2 };
//			}
//
//			finalColour.MaxToOne();
//
//			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//				static_cast<uint8_t>(finalColour.r * 255),
//				static_cast<uint8_t>(finalColour.g * 255),
//				static_cast<uint8_t>(finalColour.b * 255));
//		}
//	}
//}
//
//void Renderer::Render_W1_Part4()
//{
//	//define triangle - vertices in WOLRD space
//	bool isIntriangle{ false };
//	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
//
//	std::vector<Vertex> vertices_world
//	{
//		//Triangle 0 
//		{{0.f, 2.f, 0.f}, {1, 0, 0}},
//		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
//		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},
//		//Triangle 1
//		{{0.f, 4.f, 2.f}, {1, 0, 0}},
//		{{3.f, -2.f, 2.f}, {0, 1, 0}},
//		{{-3.f, -2.f, 2.f}, {0, 0, 1}},
//	};
//
//	std::vector<Vertex> vertices_transformed{};
//	vertices_transformed.reserve(vertices_world.size());
//
//	VertexTransformationFunction(vertices_world, vertices_transformed);
//
//	//clear back buffer
//	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
//
//	//go over triangle, per 3 vertices
//	for (int triangleIdx{}; triangleIdx < vertices_transformed.size(); triangleIdx += 3)
//	{
//		//go over each pixel is in screen space
//		for (int px{}; px < m_Width; ++px)
//		{
//			for (int py{}; py < m_Height; ++py)
//			{
//				//define current pixel in screen space
//				const Vector2 p{ px + 0.5f, py + 0.5f};
//				ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//				if (Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 1].position, p) < 0.f ||
//					Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 0].position, p) < 0.f ||
//					Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 2].position, p) < 0.f)
//				{
//					isIntriangle = false;
//				}
//				else
//				{
//					isIntriangle = true;
//				}
//
//				if (isIntriangle)
//				{
//					float w0{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 2].position, p) };
//					float w1{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 0].position, p) };
//					float w2{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 1].position, p) };
//					const float triangleArea{ w0 + w1 + w2 };
//
//					//normalize weights
//					w0 /= triangleArea;
//					w1 /= triangleArea;
//					w2 /= triangleArea;
//					
//					//interpolate depth value
//					const int bufferIdx{ px + (py * m_Width) };
//					const float interpolateDepth{ (w0 * vertices_transformed[triangleIdx + 0].position.z + 
//												  w1 * vertices_transformed[triangleIdx + 1].position.z +
//												  w2 * vertices_transformed[triangleIdx + 2].position.z) };
//					
//					if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
//					{
//						m_pDepthBufferPixels[bufferIdx] = interpolateDepth;
//
//						finalColour = { vertices_transformed[triangleIdx + 0].color * w0 +
//									    vertices_transformed[triangleIdx + 1].color * w1 +
//									    vertices_transformed[triangleIdx + 2].color * w2 };
//
//						finalColour.MaxToOne();
//
//						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//							static_cast<uint8_t>(finalColour.r * 255),
//							static_cast<uint8_t>(finalColour.g * 255),
//							static_cast<uint8_t>(finalColour.b * 255));
//					}
//				}
//			}
//		}
//	}
//}
//
//void Renderer::Render_W1_Part5()
//{
//	//define triangle - vertices in WORLD space
//	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
//
//	const std::vector<Vertex> vertices_world
//	{
//		//Triangle 0 
//		{{0.f, 2.f, 0.f}, {1, 0, 0}},
//		{{1.5f, -1.f, 0.f}, {1, 0, 0}},
//		{{-1.5f, -1.f, 0.f}, {1, 0, 0}},
//		//Triangle 1
//		{{0.f, 4.f, 2.f}, {1, 0, 0}},
//		{{3.f, -2.f, 2.f}, {0, 1, 0}},
//		{{-3.f, -2.f, 2.f}, {0, 0, 1}},
//	};
//
//	std::vector<Vertex> vertices_transformed{};
//	VertexTransformationFunction(vertices_world, vertices_transformed);
//
//	//clear back buffer
//	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
//
//	//go over triangle, per 3 vertices
//	for (int triangleIdx{}; triangleIdx < vertices_transformed.size(); triangleIdx += 3)
//	{
//		//calculate bounding box for the current triangle in screen space
//		const Vector2 v0{ vertices_transformed[triangleIdx].position.x , vertices_transformed[triangleIdx].position.y };
//		const Vector2 v1{ vertices_transformed[triangleIdx + 1].position.x,  vertices_transformed[triangleIdx + 1].position.y };
//		const Vector2 v2{ vertices_transformed[triangleIdx + 2].position.x, vertices_transformed[triangleIdx + 2].position.y };
//
//		//calculate min & max x of bounding box, clamped to screen
//		const int minX{ static_cast<int>(std::max(0.0f, std::min({ v0.x, v1.x, v2.x }))) };
//		const int maxX{ static_cast<int>(std::min(static_cast<float>(m_Width - 1), std::max({ v0.x, v1.x, v2.x }))) };
//		//calculate min & max y of bounding box, clamped to screen
//		const int minY{ static_cast<int>(std::max(0.0f, std::min({ v0.y, v1.y, v2.y }))) };
//		const int maxY{ static_cast<int>(std::min(static_cast<float>(m_Height - 1), std::max({ v0.y, v1.y, v2.y }))) };
//
//
//		//go over each pixel is in screen space
//		for (int px{minX}; px < maxX; ++px)
//		{
//			for (int py{minY}; py < maxY; ++py)
//			{
//				//define current pixel in screen space
//				const Vector2 p{ px + 0.5f, py + 0.5f };
//				ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//				if (IsPixelInTriangle(p,vertices_transformed, triangleIdx))
//				{
//					float w0{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 1].position, vertices_transformed[triangleIdx + 2].position, p) };
//					float w1{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 2].position, vertices_transformed[triangleIdx + 0].position, p) };
//					float w2{ Calculate2DCrossProduct(vertices_transformed[triangleIdx + 0].position, vertices_transformed[triangleIdx + 1].position, p) };
//					//using right formula, see slides, has performance gain too
//					const float triangleArea{ w0 + w1 + w2 };
//
//					//normalize weights
//					w0 /= triangleArea;
//					w1 /= triangleArea;
//					w2 /= triangleArea;
//
//					//interpolate depth value
//					const int bufferIdx{ px + (py * m_Width) };
//					const float interpolateDepth{ w0 * vertices_transformed[triangleIdx + 0].position.z +
//												  w1 * vertices_transformed[triangleIdx + 1].position.z +
//												  w2 * vertices_transformed[triangleIdx + 2].position.z };
//
//
//					if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
//					{
//						m_pDepthBufferPixels[bufferIdx] = interpolateDepth;
//
//						finalColour = { vertices_transformed[triangleIdx + 0].color * (w0 / triangleArea) +
//										vertices_transformed[triangleIdx + 1].color * (w1 / triangleArea) +
//										vertices_transformed[triangleIdx + 2].color * (w2 / triangleArea) };
//
//						finalColour.MaxToOne();
//
//						m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//							static_cast<uint8_t>(finalColour.r * 255),
//							static_cast<uint8_t>(finalColour.g * 255),
//							static_cast<uint8_t>(finalColour.b * 255));
//					}
//				}
//			}
//		}
//	}
//}
//
//void Renderer::Render_W2()
//{
//	//define triangle - vertices in WORLD space
//	/*std::vector<Mesh> meshes_world
//	{
//		Mesh
//		{
//			{
//				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
//				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
//				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
//				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
//				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
//				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
//				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
//				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
//				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
//			},
//			{
//				3, 0, 4, 1, 5, 2,
//				2, 6,
//				6, 3, 7, 4, 8, 5
//			},
//			PrimitiveTopology::TriangleStrip
//		}
//	};*/
//
//	std::vector<Mesh> meshes_world 
//	{
//		Mesh
//		{
//			{
//				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
//				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
//				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
//				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
//				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
//				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
//				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
//				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
//				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
//			},
//			{
//				3, 0, 1,    1, 4, 3,    4, 1, 2,
//				2, 5, 4,    6, 3, 4,    4, 7, 6,
//				7, 4, 5,    5, 8, 7
//			},
//			PrimitiveTopology::TriangleList
//		}
//	};
//
//	//std::vector<Mesh> mesh_transformed{};
//	//mesh_transformed.reserve(meshes_world.size());
//
//	VertexTransformationFunction(meshes_world);
//
//	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
//
//	//clear back buffer
//	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
//
//	for (Mesh mesh : meshes_world)
//	{
//		if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
//		{
//			//extra variable; amount of sides : 0, 1, 2
//			const auto& maxIdx{ mesh.indices.size() - 2 };
//
//			//go over triangle, per 3 vertices
//			for (int triangleIdx{}; triangleIdx < maxIdx; ++triangleIdx)
//			{
//				TriangleHandeling(triangleIdx, mesh);
//			}
//		}
//		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
//		{
//			//go over triangle, per 3 vertices
//			for (int triangleIdx{}; triangleIdx < mesh.indices.size(); triangleIdx += 3)
//			{
//				TriangleHandeling(triangleIdx, mesh);
//			}
//		}		
//	}
//
//	//clear vertices
//	meshes_world.clear();
//}
//
//void Renderer::RenderQuad_W3()
//{
//	//define triangle - vertices in WORLD space
//	/*std::vector<Mesh> meshes_world
//	{
//		Mesh
//		{
//			{
//				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
//				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
//				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
//				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
//				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
//				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
//				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
//				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
//				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
//			},
//			{
//				3, 0, 4, 1, 5, 2,
//				2, 6,
//				6, 3, 7, 4, 8, 5
//			},
//			PrimitiveTopology::TriangleStrip
//		}
//	};*/
//
//	std::vector<Mesh> meshes_world
//	{
//		Mesh
//		{
//			{
//				Vertex{ {-3, 3, -2}, {}, {0.f, 0.f} },
//				Vertex{ {0, 3, -2}, {}, {0.5f, 0.f} },
//				Vertex{ {3, 3, -2}, {}, {1.f, 0.f} },
//				Vertex{ {-3, 0, -2}, {}, {0.f, 0.5f} },
//				Vertex{ {0, 0, -2}, {}, {0.5f, 0.5f} },
//				Vertex{ {3, 0, -2}, {}, {1.f, 0.5f} },
//				Vertex{ {-3, -3, -2}, {}, {0.f, 1.f} },
//				Vertex{ {0, -3, -2}, {}, {0.5f, 1.f} },
//				Vertex{ {3, -3, -2}, {}, {1.f, 1.f} }
//			},
//			{
//				3, 0, 1,    1, 4, 3,    4, 1, 2,
//				2, 5, 4,    6, 3, 4,    4, 7, 6,
//				7, 4, 5,    5, 8, 7
//			},
//			PrimitiveTopology::TriangleList
//		}
//	};
//
//	VertexTransformationFunction(meshes_world);
//
//	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
//
//	//clear back buffer
//	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
//
//	for (Mesh mesh : meshes_world)
//	{
//		if (mesh.primitiveTopology == PrimitiveTopology::TriangleStrip)
//		{
//			//extra variable; amount of sides : 0, 1, 2
//			const auto& maxIdx{ mesh.indices.size() - 2 };
//
//			//go over triangle, per 3 vertices
//			for (int triangleIdx{}; triangleIdx < maxIdx; ++triangleIdx)
//			{
//				TriangleHandeling(triangleIdx, mesh);
//			}
//		}
//		else if (mesh.primitiveTopology == PrimitiveTopology::TriangleList)
//		{
//			//go over triangle, per 3 vertices
//			for (int triangleIdx{}; triangleIdx < mesh.indices.size(); triangleIdx += 3)
//			{
//				TriangleHandeling(triangleIdx, mesh);
//			}
//		}
//	}
//}
//
//void Renderer::RenderMesh_W3()
//{
//	//from world to view to projection to screen space
//	VertexTransformationFunction(m_MeshesObject);
//
//	std::fill_n(m_pDepthBufferPixels, m_Width * m_Height, FLT_MAX);
//
//	//clear back buffer
//	SDL_FillRect(m_pBackBuffer, &m_pBackBuffer->clip_rect, SDL_MapRGB(m_pBackBuffer->format, 0, 0, 0));
//
//	for (Mesh& mesh : m_MeshesObject)
//	{		
//		//go over triangle, per 3 vertices
//		for (int triangleIdx{}; triangleIdx < mesh.indices.size(); triangleIdx += 3)
//		{
//			TriangleHandeling(triangleIdx, mesh);
//		}
//	}
//}

//bool Renderer::IsPixelInTriangle(const Vector2& p, const std::vector<Vertex>& vertex, const int index)
//{
//	if (Calculate2DCrossProduct(vertex[index + 2].position, vertex[index + 1].position, p) < 0.f)
//	{
//		return false;
//	}
//	else if (Calculate2DCrossProduct(vertex[index + 1].position, vertex[index + 0].position, p) < 0.f)
//	{
//		return false;
//	}
//	else if (Calculate2DCrossProduct(vertex[index + 0].position, vertex[index + 2].position, p) < 0.f)
//	{
//		return false;
//	}
//
//	return true;
//}
//
//void Renderer::PixelHandeling(int px, int py, int triangleIdx, const std::vector<Vertex>& vertex_transformed)
//{
//	//define current pixel in screen space
//	const Vector2 p{ px + 0.5f, py + 0.5f };
//	ColorRGB finalColour{ 0.f, 0.f, 0.f };
//
//	if (IsPixelInTriangle(p, vertex_transformed, triangleIdx))
//	{
//		float w0{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 1].position, vertex_transformed[triangleIdx + 2].position, p) };
//		float w1{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 2].position, vertex_transformed[triangleIdx + 0].position, p) };
//		float w2{ Calculate2DCrossProduct(vertex_transformed[triangleIdx + 0].position, vertex_transformed[triangleIdx + 1].position, p) };
//		//using right formula, see slides, has performance gain too
//		const float triangleArea{ w0 + w1 + w2 };
//
//		//normalize weights
//		w0 /= triangleArea;
//		w1 /= triangleArea;
//		w2 /= triangleArea;
//
//		//interpolate depth value
//		const int bufferIdx{ px + (py * m_Width) };
//		const float interpolateDepth{ (w0 * vertex_transformed[triangleIdx + 0].position.z +
//									   w1 * vertex_transformed[triangleIdx + 1].position.z +
//									   w2 * vertex_transformed[triangleIdx + 2].position.z) };
//
//
//		if (interpolateDepth < m_pDepthBufferPixels[bufferIdx])
//		{
//			m_pDepthBufferPixels[bufferIdx] = interpolateDepth;
//
//			finalColour = { vertex_transformed[triangleIdx + 0].color * w0 +
//							vertex_transformed[triangleIdx + 1].color * w1 +
//							vertex_transformed[triangleIdx + 2].color * w2 };
//
//			finalColour.MaxToOne();
//
//			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
//				static_cast<uint8_t>(finalColour.r * 255),
//				static_cast<uint8_t>(finalColour.g * 255),
//				static_cast<uint8_t>(finalColour.b * 255));
//		}
//	}
//}

//float Renderer::Calculate2DCrossProduct(const Vector3& a, const Vector3& b, const Vector2& c)
//{
//	return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
//}

//void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
//{	
//	for (int idx{}; idx < vertices_in.size(); ++idx)
//	{
//		const Vector3 worldToView{ m_Camera.viewMatrix.TransformPoint(vertices_in[idx].position) };
//		Vector3 projectedVertex{};
//
//		projectedVertex.x = (worldToView.x / worldToView.z) / (float(m_Width) / float(m_Height) * m_Camera.fov);
//		projectedVertex.y = (worldToView.y / worldToView.z) / m_Camera.fov;
//		projectedVertex.z = worldToView.z;
//
//		projectedVertex.x = ((projectedVertex.x + 1) / 2) * m_Width;
//		projectedVertex.y = ((1 - projectedVertex.y) / 2) * m_Height;
//
//		vertices_out.emplace_back(Vertex{ projectedVertex, vertices_in[idx].color });
//	}
//}