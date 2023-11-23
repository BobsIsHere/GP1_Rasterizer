#pragma once
#include <cassert>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Maths.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle):
			origin{_origin},
			fovAngle{_fovAngle}
		{
		}


		Vector3 origin{};
		float fovAngle{90.f};
		float fov{ tanf((fovAngle * TO_RADIANS) / 2.f) };
		float aspectRatio{};

		Vector3 forward{Vector3::UnitZ};
		Vector3 up{Vector3::UnitY};
		Vector3 right{Vector3::UnitX};

		float totalPitch{};
		float totalYaw{};
		float speed{};

		Matrix invViewMatrix{};
		Matrix viewMatrix{};
		Matrix projectionMatrix{};

		void Initialize(float ratio, float _fovAngle = 90.f, Vector3 _origin = {0.f,0.f,0.f})
		{
			fovAngle = _fovAngle;
			fov = tanf((fovAngle * TO_RADIANS) / 2.f);
			aspectRatio = ratio;

			origin = _origin;

			speed = 10.f;
		}

		void CalculateViewMatrix()
		{
			//ONB => invViewMatrix
			//Inverse(ONB) => ViewMatrix
			Matrix rotation{ Matrix::CreateRotationX(- totalPitch * TO_RADIANS) * Matrix::CreateRotationY(totalYaw * TO_RADIANS) };

			forward = rotation.TransformVector(Vector3::UnitZ);
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			invViewMatrix = {
				Vector4{right, 0},
				Vector4{up, 0},
				Vector4{forward, 0},
				Vector4{origin, 1}
			};

			//ViewMatrix => Matrix::CreateLookAtLH(...) [not implemented yet]
			//viewMatrix = Matrix::CreateLookAtLH(origin, forward, up);
			viewMatrix = invViewMatrix.Inverse();
		}

		void CalculateProjectionMatrix()
		{
			//ProjectionMatrix => Matrix::CreatePerspectiveFovLH(...)
			//DirectX Implementation => https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectivefovlh
			const float near{ 0.1f };
			const float far{ 100.f };

			projectionMatrix = Matrix::CreatePerspectiveFovLH(fov, aspectRatio, near, far);
		}

		void Update(Timer* pTimer)
		{
			const float deltaTime = pTimer->GetElapsed();

			//keyboard input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			if (pKeyboardState[SDL_SCANCODE_W])
			{
				origin += forward * speed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * speed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_D])
			{
				origin += right * speed * deltaTime;
			}
			else if (pKeyboardState[SDL_SCANCODE_A])
			{
				origin -= right * speed * deltaTime;
			}

			//mouse input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			if ((mouseState & SDL_BUTTON_RMASK) != 0 && !(mouseState & SDL_BUTTON_LMASK) != 0)
			{
				// Right mouse button is pressed
				totalYaw += mouseX;
				totalPitch += mouseY;
			}

			if ((mouseState & SDL_BUTTON_LMASK) != 0 && (mouseState & SDL_BUTTON_RMASK) != 0)
			{
				origin += up * speed * float(mouseY) * deltaTime;
			}
			else if ((mouseState & SDL_BUTTON_LMASK) != 0)
			{
				if (mouseY < 0)
				{
					origin += forward * speed * deltaTime;
				}
				else if (mouseY > 0)
				{
					origin -= forward * speed * deltaTime;
				}

				// Right mouse button is pressed
				totalYaw += mouseX;
			}

			//Update Matrices
			CalculateViewMatrix();
			CalculateProjectionMatrix(); //Try to optimize this - should only be called once or when fov/aspectRatio changes
		}
	};
}
