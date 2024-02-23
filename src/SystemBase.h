#pragma once

enum class SystemType : size_t
{
	GRAPHICS = 0,
	CAMERA,
	MAX
};

// Pure virtual class that defines the most basic system class
class SystemBase
{
	public:
		virtual void init() = 0;
		virtual void update(float dt) = 0;
		virtual void shutdown() = 0;

		virtual SystemType Type() const = 0;
};