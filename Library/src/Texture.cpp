#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>
#include <memory>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_SurfacePtr{ pSurface },
		m_SurfacePixelsPtr{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_SurfacePtr)
		{
			SDL_FreeSurface(m_SurfacePtr);
			m_SurfacePtr = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//Load SDL_Surface using IMG_LOAD
		//Create & Return a new Texture Object (using SDL_Surface)
		//use c_str() to obtain const char*
		SDL_Surface* texturePtr{ IMG_Load(path.data())};

		//if IMG_Load fails -> function will return nullptr
		if (!texturePtr)
		{
			return nullptr;
		}

		return new Texture{texturePtr};
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Sample the correct texel for the given uv
		const float px{ m_SurfacePtr->w * uv.x };
		const float py{ m_SurfacePtr->h * uv.y };
		const uint32_t pIndex{ static_cast<uint32_t>(px) + (static_cast<uint32_t>(py) * m_SurfacePtr->w) };

		uint8_t r{};
		uint8_t g{};
		uint8_t b{};

		SDL_GetRGB(m_SurfacePixelsPtr[pIndex], m_SurfacePtr->format, &r, &g, &b);

		float rFloat{ r / 255.f };
		float gFloat{ g / 255.f };
		float bFloat{ b / 255.f };

		return ColorRGB{ rFloat,gFloat,bFloat };
	}
}