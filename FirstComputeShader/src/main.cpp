#include "pch.hpp"
using Microsoft::WRL::ComPtr;

// Window
GLFWwindow* window;
const uint32_t width = 1920;
const uint32_t height = 1050;
const char* title = "Hello d3d12 compute shaders";

// DXGI
ComPtr<IDXGIFactory4> dxgi_factory;
ComPtr<IDXGISwapChain4> swapchain;
const uint32_t buffer_count = 2;
uint32_t buffer_index;


// D3D12 core interfaces
ComPtr<ID3D12Debug6> debug_controller;
ComPtr<ID3D12Device2> device;
// Command interfaces
ComPtr<ID3D12CommandQueue> direct_command_queue;
ComPtr<ID3D12CommandAllocator> command_allocator;
ComPtr<ID3D12GraphicsCommandList> command_list;

// Synchronization
ComPtr<ID3D12Fence> fence;
uint64_t fence_value;
HANDLE fence_event;


// GPU Resources
ComPtr<ID3D12Resource> swapchain_buffers[buffer_count];
ComPtr<ID3D12Resource> framebuffer;

// Shader layout and pipeline state
ComPtr<ID3D12RootSignature> root_signature;
ComPtr<ID3D12PipelineState> pso;

// Resource descriptors(views)
ComPtr<ID3D12DescriptorHeap> descriptor_heap;



int main()
{
#pragma region window_init
	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}
#pragma endregion

#pragma region d3d12_init_core
	/* Get d3d12 debug layer and upgrade it to our version(ComPtr<ID3D12Debug6> debug_controller);
	 * And enable validations(has to be done before device creation)
	 */
	ComPtr<ID3D12Debug> debug_controller_tier0;
	EXC_( D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller_tier0)));
	EXC_( debug_controller_tier0->QueryInterface(IID_PPV_ARGS(&debug_controller)));
	debug_controller->SetEnableSynchronizedCommandQueueValidation(true);
	debug_controller->SetForceLegacyBarrierValidation(true);
	debug_controller->SetEnableAutoName(true);
	debug_controller->EnableDebugLayer();
	debug_controller->SetEnableGPUBasedValidation(true);


	// Passing nullptr means its up to system which adapter to use for device, might even use WARP(no gpu)
	EXC_( D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));


	const D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc{
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		1,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	};
	EXC_( device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap)));


	const D3D12_COMMAND_QUEUE_DESC command_queue_desc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		0
	};
	EXC_( device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&direct_command_queue)));


	EXC_( device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&command_allocator)));


	EXC_( device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		command_allocator.Get(), nullptr, IID_PPV_ARGS(&command_list)));

	EXC_( command_list->Close());
	EXC_( command_allocator->Reset());
	EXC_( command_list->Reset(command_allocator.Get(), nullptr));


	fence_value = 0;
	EXC_( device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fence_event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
	if (!fence_event) 
	{
		EXC_( HRESULT_FROM_WIN32(GetLastError()));
	}
#pragma endregion

#pragma region d3d12_init_swapchain
	// Factory to create swapchains etc
	EXC_( CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory)));
	
	const DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
		width,
		height,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		FALSE,
		{1, 0},
		DXGI_USAGE_BACK_BUFFER,
		buffer_count,
		DXGI_SCALING_STRETCH,
		DXGI_SWAP_EFFECT_FLIP_DISCARD,
		DXGI_ALPHA_MODE_UNSPECIFIED,
		0
	};
	// Create and upgrade swapchain to our version(ComPtr<IDXGISwapChain4> swapchain)
	ComPtr<IDXGISwapChain1> swapchain_tier_dx12;
	EXC_( dxgi_factory->CreateSwapChainForHwnd(
		direct_command_queue.Get(),
		glfwGetWin32Window(window),
		&swapchain_desc,
		nullptr,
		nullptr,
		&swapchain_tier_dx12));
	EXC_( swapchain_tier_dx12.As(&swapchain));

	// Get swapchain pointers to ID3D12Resource's that represents buffers
	for (int i = 0; i < buffer_count; i++)
	{
		EXC_( swapchain->GetBuffer(i, IID_PPV_ARGS(&swapchain_buffers[i])));
		EXC_( swapchain_buffers[i]->SetName(L"swapchain_buffer"));
	}
#pragma endregion

#pragma region d3d12_init_shader_resources
	// Retrieve swapchain buffer description and create identical resource but with UAV allowed, so compute shader could write to it
	auto buffer_desc = swapchain_buffers[0]->GetDesc();
	buffer_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	const CD3DX12_HEAP_PROPERTIES default_heap_props{ D3D12_HEAP_TYPE_DEFAULT };

	EXC_( device->CreateCommittedResource(
		&default_heap_props, D3D12_HEAP_FLAG_NONE,
		&buffer_desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr, IID_PPV_ARGS(&framebuffer)));

	EXC_( framebuffer->SetName(L"framebuffer"));
	/* Create view for our framebuffer so compute shader can access it 
	*  passing UAV desc is not neccessary? it's determined automatically by system(correctly)
	*  using first handle/descriptor in the heap as currently we dont have any other resources
	*/
	device->CreateUnorderedAccessView(framebuffer.Get(), nullptr, nullptr, descriptor_heap->GetCPUDescriptorHandleForHeapStart());

	
	// Shader and its layout
	ComPtr<ID3DBlob> cs_rendering;
	EXC_( D3DReadFileToBlob(L"rendering.cso", &cs_rendering));
	ComPtr<ID3DBlob> rootsig;
	EXC_( D3DReadFileToBlob(L"rootsig.cso", &rootsig));

	EXC_( device->CreateRootSignature(0, rootsig->GetBufferPointer(), rootsig->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

	D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {};
	pso_desc.pRootSignature = root_signature.Get();
	pso_desc.CS.BytecodeLength = cs_rendering->GetBufferSize();
	pso_desc.CS.pShaderBytecode = cs_rendering->GetBufferPointer();
	EXC_( device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pso)));
#pragma endregion


	while (!glfwWindowShouldClose(window))
	{
		buffer_index = swapchain->GetCurrentBackBufferIndex();
		const auto& backbuffer = swapchain_buffers[buffer_index];


		command_list->SetPipelineState(pso.Get());
		command_list->SetComputeRootSignature(root_signature.Get());
		command_list->SetDescriptorHeaps(1, descriptor_heap.GetAddressOf());


		command_list->SetComputeRootDescriptorTable(0, descriptor_heap->GetGPUDescriptorHandleForHeapStart());
		
		command_list->Dispatch(1920 / 8, 1080 / 8, 1);


		const auto fb_precopy = CD3DX12_RESOURCE_BARRIER::Transition(
			framebuffer.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
		command_list->ResourceBarrier(1, &fb_precopy);

		const auto bb_precopy = CD3DX12_RESOURCE_BARRIER::Transition(
			backbuffer.Get(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		command_list->ResourceBarrier(1, &bb_precopy);


		command_list->CopyResource(backbuffer.Get(), framebuffer.Get());


		const auto fb_postcopy = CD3DX12_RESOURCE_BARRIER::Transition(
			framebuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		command_list->ResourceBarrier(1, &fb_postcopy);

		const auto bb_postcopy = CD3DX12_RESOURCE_BARRIER::Transition(
			framebuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		command_list->ResourceBarrier(1, &bb_postcopy);



		EXC_( command_list->Close());
		ID3D12CommandList* const command_lists[] = { command_list.Get() };
		direct_command_queue->ExecuteCommandLists(sizeof(command_lists) / sizeof command_lists[0], command_lists);

		EXC_( direct_command_queue->Signal(fence.Get(), ++fence_value));
		EXC_( fence->SetEventOnCompletion(fence_value, fence_event));
		if (::WaitForSingleObject(fence_event, INFINITE) == WAIT_FAILED) {
			EXC_( HRESULT_FROM_WIN32(GetLastError()));
		}

		EXC_( command_allocator->Reset());
		EXC_( command_list->Reset(command_allocator.Get(), nullptr));


		EXC_( swapchain->Present(0, 0));

		glfwPollEvents();
	}



	EXC_( direct_command_queue->Signal(fence.Get(), fence_value));
	EXC_( fence->SetEventOnCompletion(fence_value, fence_event));
	if (::WaitForSingleObject(fence_event, 2000) == WAIT_FAILED)
	{
		EXC_( HRESULT_FROM_WIN32(GetLastError()));
	}

	glfwTerminate();
	std::cin.get();
	return 0;
}