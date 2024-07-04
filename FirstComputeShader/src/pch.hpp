#pragma once
// std
#include <iostream>
#include <sstream>
#include <format>
#include <cstdint>
#include <stdexcept>

// glfw
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

// Dx12/Win
#include <wrl.h>

#include <d3dx12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

// Utils
#include "utils/hresult.hpp"