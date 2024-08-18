#pragma once

// oder should follows the update order
enum class SystemType : size_t
{
	INPUT = 0,
	CAMERA,
	GRAPHICS,
	MAX
};

// Pure virtual class that defines the most basic system class
class SystemBase
{
	public:
		virtual void init() = 0;
		virtual void update(float dt) = 0;
		virtual void shutdown() = 0;
		// tell what type of sub class it is
		virtual SystemType Type() const = 0;
};