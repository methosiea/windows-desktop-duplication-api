#include "WindowsDesktopDuplicationManager.h"
#include <cassert>
#ifdef _DEBUG
#include <iostream>
#endif
#include <winnt.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include "macros.h"

WindowsDesktopDuplicationManager::WindowsDesktopDuplicationManager()
{
	// Create a new factory.
	ASSERT_SUCCEEDED(
		CreateDXGIFactory1(IID_PPV_ARGS(&m_pFactory)),
		"Failed to create a new factory."
	);

	// Get the primary adapter.
	// ---
	// The adapter with which the desktop is primarily displayed is the first one with index 0.
	ASSERT_SUCCEEDED(
		m_pFactory->EnumAdapters1(0, &m_pPrimaryAdapter),
		"Failed to get the primary adapter."
	);

	// Get the primary output.
	// ---
	// The output with which the desktop is primarily displayed is the first one with index 0.
	// ---
	// Here QueryInterface is omitted and reinterpret_cast is used instead, since this can
	// be evaluated at compile time.
	ASSERT_SUCCEEDED(
		m_pPrimaryAdapter->EnumOutputs(0, reinterpret_cast<IDXGIOutput**>(&m_pPrimaryOutput)),
		"Failed to get the primary output."
	);

#ifdef _DEBUG
	// Get a description of the primary output.
	DXGI_OUTPUT_DESC desc;

	ASSERT_SUCCEEDED(
		m_pPrimaryOutput->GetDesc(&desc),
		"Failed to get a description of the primary output."
	);

	// Expects the screen output to have no offset.
	assert(desc.DesktopCoordinates.top == 0);
	assert(desc.DesktopCoordinates.left == 0);

	// Expects that the screen output is not rotated.
	assert(desc.Rotation == DXGI_MODE_ROTATION_IDENTITY);
#endif

	// Create a new device.
	ASSERT_SUCCEEDED(
		D3D11CreateDevice(
			// pAdapter
			// A pointer to the video adapter to use when creating a device. Pass NULL
			// to use the default adapter, which is the first adapter that is enumerated by
			// IDXGIFactory1::EnumAdapters.
			m_pPrimaryAdapter,
			// DriverType
			// The D3D_DRIVER_TYPE, which represents the driver type to create.
			D3D_DRIVER_TYPE_UNKNOWN,
			// Software
			// A handle to a DLL that implements a software rasterizer.If DriverType is
			// D3D_DRIVER_TYPE_SOFTWARE, Software must not be NULL.Get the
			// handle by calling LoadLibrary, LoadLibraryEx, or GetModuleHandle.
			NULL,
			// Flags
			// The runtime layers to enable (see D3D11_CREATE_DEVICE_FLAG); values
			// can be bitwise OR'd together.
			0,
			// pFeatureLevels
			// A pointer to an array of D3D_FEATURE_LEVELs, which determine the order
			// of feature levels to attempt to create. If pFeatureLevels is set to NULL, this
			// function uses the following array of feature levels:
			// 
			// {
			//     D3D_FEATURE_LEVEL_11_0,
			//     D3D_FEATURE_LEVEL_10_1,
			//     D3D_FEATURE_LEVEL_10_0,
			//     D3D_FEATURE_LEVEL_9_3,
			//     D3D_FEATURE_LEVEL_9_2,
			//     D3D_FEATURE_LEVEL_9_1,
			// };
			// 
			// Note: If the Direct3D 11.1 runtime is present on the computer and
			// pFeatureLevels is set to NULL, this function won't create a
			// D3D_FEATURE_LEVEL_11_1 device. To create a
			// D3D_FEATURE_LEVEL_11_1 device, you must explicitly provide a
			// D3D_FEATURE_LEVEL array that includes D3D_FEATURE_LEVEL_11_1.
			// If you provide a D3D_FEATURE_LEVEL array that contains
			// D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have the
			// Direct3D 11.1 runtime installed, this function immediately fails with
			// E_INVALIDARG.
			NULL,
			// FeatureLevels
			// The number of elements in pFeatureLevels.
			0,
			// SDKVersion
			// The SDK version; use D3D11_SDK_VERSION.
			D3D11_SDK_VERSION,
			// ppDevice
			// Returns the address of a pointer to an ID3D11Device object that
			// represents the device created. If this parameter is NULL, no ID3D11Device
			// will be returned.
			&m_pDevice,
			// pFeatureLevel
			// If successful, returns the first D3D_FEATURE_LEVEL from the pFeatureLevels
			// array which succeeded.Supply NULL as an input if you don't need to
			// determine which feature level is supported.
			NULL,
			// ppImmediateContext
			// Returns the address of a pointer to an ID3D11DeviceContext object that
			// represents the device context. If this parameter is NULL, no
			// ID3D11DeviceContext will be returned.
			&m_pImmediateContext
		),
		"Failed to create a new device."
	);

	InitializeOutputDuplication();

#ifdef _DEBUG
	std::cout << "The desktop duplication manager was successfully initialized." << std::endl;
#endif
}

void WindowsDesktopDuplicationManager::Update()
{
	assert(m_pOutputDuplication != NULL);

	ID3D11Texture2D* pDesktopSurface = NULL;

#ifdef _DEBUG
	std::cout << "Acquires the next frame." << std::endl;
#endif

	const HRESULT hr = m_pOutputDuplication->AcquireNextFrame(
		// TimeoutInMilliseconds
		// The time-out interval, in milliseconds. This interval specifies the amount of time that this
		// method waits for a new frame before it returns to the caller.This method returns if the
		// interval elapses, and a new desktop image is not available.
		1000,
		// pFrameInfo
		// A pointer to a memory location that receives the DXGI_OUTDUPL_FRAME_INFO structure
		// that describes timing and presentation statistics for a frame.
		&m_frameInfo,
		// ppDesktopResource
		// A pointer to a variable that receives the IDXGIResource interface of the surface that
		// contains the desktop bitmap.
		reinterpret_cast<IDXGIResource**>(&pDesktopSurface)
	);

	// DXGI_ERROR_ACCESS_LOST
	// If the desktop duplication interface is invalid. The desktop duplication interface typically
	// becomes invalid when a different type of image is displayed on the desktop. Examples of this
	// situation are:
	// - Desktop switch
	// - Mode change
	// Switch from DWM on, DWM off, or other full - screen application
	// In this situation, the application must release the IDXGIOutputDuplication interface and create
	// a new IDXGIOutputDuplication for the new content.
	if (hr == DXGI_ERROR_ACCESS_LOST)
	{
#ifdef _DEBUG
		std::cout << "The output duplication has lost access." << std::endl;
		std::cout << "The output duplication gets released and reinitialized." << std::endl;
#endif

		ReleaseOutputDuplication();
		InitializeOutputDuplication();

		return;
	}

	assert(hr != DXGI_ERROR_INVALID_CALL);
	assert(hr != E_INVALIDARG);

	if (FAILED(hr))
	{
#ifdef _DEBUG
		std::cout << "Failed to acquire the next frame." << std::endl;
#endif

		return;
	}

	// Check if the pointer visibility has changed.
	static auto bLastVisibility = TRUE;
	const auto bVisibility = m_frameInfo.PointerPosition.Visible;

	if (bVisibility != bLastVisibility)
	{
		OnPointerVisibilityChanged();
	}

	// Check if the pointer shape has changed.
	// ---
	// A non-zero LastMouseUpdateTime indicates an update to either a mouse
	// pointer position or a mouse pointer position and shape. That is, the
	// mouse pointer position is always valid for a non-zero
	// LastMouseUpdateTime; however, the application must check the value of
	// the PointerShapeBufferSize member to determine whether the shape was
	// updated too.
	const auto iPointerShapeBufferSize = m_frameInfo.PointerShapeBufferSize;

	if (bVisibility && m_frameInfo.LastMouseUpdateTime.QuadPart != 0 && iPointerShapeBufferSize != 0)
	{
		OnPointerShapeChanged();
	}

	// Check if the desktop image has changed.
	if (m_frameInfo.LastPresentTime.QuadPart != 0 && m_frameInfo.TotalMetadataBufferSize != 0)
	{
		OnDesktopImageChanged();
	}

	// Release current frame.
	m_pOutputDuplication->ReleaseFrame();

	pDesktopSurface->Release();
	pDesktopSurface = NULL;
}

void WindowsDesktopDuplicationManager::Release()
{
	assert(m_pFactory != NULL);
	assert(m_pPrimaryAdapter != NULL);
	assert(m_pPrimaryOutput != NULL);
	assert(m_pDevice != NULL);
	assert(m_pImmediateContext != NULL);

	m_pFactory->Release();
	m_pFactory = NULL;

	m_pPrimaryAdapter->Release();
	m_pPrimaryAdapter = NULL;

	m_pPrimaryOutput->Release();
	m_pPrimaryOutput = NULL;

	m_pDevice->Release();
	m_pDevice = NULL;

	m_pImmediateContext->Release();
	m_pImmediateContext = NULL;

	ReleaseOutputDuplication();
	ReleasePointerShapeBuffer();
	ReleaseMetadataBuffer();
}

void WindowsDesktopDuplicationManager::InitializeOutputDuplication()
{
	assert(m_pPrimaryOutput != NULL);
	assert(m_pDevice != NULL);
	assert(m_pOutputDuplication == NULL);

	// Duplicate the output.
	// ---
	// Using the original DuplicateOutput function always converts the fullscreen surface to a
	// 32-bit BGRA format. In cases where the current fullscreen application is using a
	// different buffer format, a conversion to 32-bit BGRA incurs a performance penalty.
	ASSERT_SUCCEEDED(
		m_pPrimaryOutput->DuplicateOutput(m_pDevice, &m_pOutputDuplication),
		"Failed to duplicate the output."
	);

	// Get a description of the output duplication.
	m_pOutputDuplication->GetDesc(&m_outputDuplicationDesc);
}

void WindowsDesktopDuplicationManager::ReleaseOutputDuplication()
{
	assert(m_pOutputDuplication != NULL);

	m_pOutputDuplication->Release();
	m_pOutputDuplication = NULL;
}

void WindowsDesktopDuplicationManager::ReleasePointerShapeBuffer()
{
	if (m_pPointerShapeBuffer != NULL)
	{
		delete[] m_pPointerShapeBuffer;
		m_pPointerShapeBuffer = NULL;
	}

	m_iPointerShapeBufferSize = 0;
}

void WindowsDesktopDuplicationManager::ReleaseMetadataBuffer()
{
	if (m_pMetadataBuffer != NULL)
	{
		delete[] m_pMetadataBuffer;
		m_pMetadataBuffer = NULL;
	}

	m_iMetadataBufferSize = 0;

	m_pDirtyRectsBegin = NULL;
	m_iDirtyRectsCount = 0;

	m_pMoveRectsBegin = NULL;
	m_iMoveRectsCount = 0;
}

void WindowsDesktopDuplicationManager::OnPointerVisibilityChanged()
{
#ifdef _DEBUG
	std::cout << "The pointer visibility has changed." << std::endl;
#endif
}

void WindowsDesktopDuplicationManager::OnPointerShapeChanged()
{
#ifdef _DEBUG
	std::cout << "The pointer shape has changed." << std::endl;
#endif

	UpdatePointerShapeBuffer();
}

void WindowsDesktopDuplicationManager::OnDesktopImageChanged()
{
#ifdef _DEBUG
	std::cout << "The desktop image has changed." << std::endl;
#endif

	UpdateMetadataBuffer();

#ifdef _DEBUG
	std::cout << "Number of moved rects: " << m_iMoveRectsCount << std::endl;
	std::cout << "Number of dirty rects: " << m_iDirtyRectsCount << std::endl;
#endif

	for (UINT i = 0; i < m_iMoveRectsCount; ++i)
	{
		const auto moveRect = m_pMoveRectsBegin[i];

#ifdef _DEBUG
		std::cout << "Move Rect:" << std::endl;
		std::cout << "- Source - X = " << moveRect.SourcePoint.x << std::endl;
		std::cout << "- Source - Y = " << moveRect.SourcePoint.x << std::endl;
		std::cout << "- Destination - Top    = " << moveRect.DestinationRect.top << std::endl;
		std::cout << "- Destination - Right  = " << moveRect.DestinationRect.right << std::endl;
		std::cout << "- Destination - Bottom = " << moveRect.DestinationRect.bottom << std::endl;
		std::cout << "- Destination - Left   = " << moveRect.DestinationRect.left << std::endl;
#endif
	}

	for (UINT i = 0; i < m_iDirtyRectsCount; ++i)
	{
		const auto dirtyRect = m_pDirtyRectsBegin[i];

#ifdef _DEBUG
		std::cout << "Dirty Rect:" << std::endl;
		std::cout << "- Top    = " << dirtyRect.top << std::endl;
		std::cout << "- Right  = " << dirtyRect.right << std::endl;
		std::cout << "- Bottom = " << dirtyRect.bottom << std::endl;
		std::cout << "- Left   = " << dirtyRect.left << std::endl;
#endif
	}
}

void WindowsDesktopDuplicationManager::UpdatePointerShapeBuffer()
{
	const auto iPointerShapeBufferSize = m_frameInfo.PointerShapeBufferSize;

	assert(iPointerShapeBufferSize > 0);

	// Check if the current buffer is to small.
	if (iPointerShapeBufferSize > m_iPointerShapeBufferSize)
	{
		ReleasePointerShapeBuffer();

		m_pPointerShapeBuffer = new (std::nothrow) BYTE[iPointerShapeBufferSize];
		m_iPointerShapeBufferSize = iPointerShapeBufferSize;

		assert(("Failed to allocate memory for m_pPointerShapeBuffer.", m_pPointerShapeBuffer != NULL));

		if (m_pPointerShapeBuffer == NULL)
		{
#ifdef _DEBUG
			std::cout << "m_pPointerShapeBuffer gets released." << std::endl;
#endif

			ReleasePointerShapeBuffer();

			return;
		}
	}

	assert(m_pPointerShapeBuffer != NULL);

	ASSERT_SUCCEEDED(
		m_pOutputDuplication->GetFramePointerShape(
			m_iPointerShapeBufferSize,
			m_pPointerShapeBuffer,
			&m_iPointerShapeBufferSize,
			&m_pointerShapeInfo
		),
		"Failed to get move rects."
	);
}

void WindowsDesktopDuplicationManager::UpdateMetadataBuffer()
{
	const auto iMetadataBufferSize = m_frameInfo.TotalMetadataBufferSize;

	assert(iMetadataBufferSize > 0);

	// Check if the current buffer is to small.
	if (iMetadataBufferSize > m_iMetadataBufferSize)
	{
		ReleaseMetadataBuffer();

		m_pMetadataBuffer = new (std::nothrow) BYTE[iMetadataBufferSize];
		m_iMetadataBufferSize = iMetadataBufferSize;

		assert(("Failed to allocate memory for m_byMetadataBuffer.", m_pMetadataBuffer != NULL));

		if (m_pMetadataBuffer == NULL)
		{
#ifdef _DEBUG
			std::cout << "m_byMetadataBuffer gets cleared." << std::endl;
#endif

			ReleaseMetadataBuffer();

			return;
		}
	}

	assert(m_pMetadataBuffer != NULL);

	UINT iMoveRectsBufferSize;
	m_pMoveRectsBegin = reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_pMetadataBuffer + 0);

	ASSERT_SUCCEEDED(
		m_pOutputDuplication->GetFrameMoveRects(m_iMetadataBufferSize, m_pMoveRectsBegin, &iMoveRectsBufferSize),
		"Failed to get move rects."
	);

	UINT iDirtyRectsBufferSize;
	m_pDirtyRectsBegin = reinterpret_cast<RECT*>(m_pMetadataBuffer + iMoveRectsBufferSize);

	ASSERT_SUCCEEDED(
		m_pOutputDuplication->GetFrameDirtyRects(
			m_iMetadataBufferSize - iMoveRectsBufferSize,
			m_pDirtyRectsBegin,
			&iDirtyRectsBufferSize
		),
		"Failed to get dirty rects."
	);

	m_iMoveRectsCount = iMoveRectsBufferSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);
	m_iDirtyRectsCount = iDirtyRectsBufferSize / sizeof(RECT);
}

WindowsDesktopDuplicationManager::~WindowsDesktopDuplicationManager()
{
	assert(m_pFactory == NULL);
	assert(m_pPrimaryAdapter == NULL);
	assert(m_pPrimaryOutput == NULL);
	assert(m_pOutputDuplication == NULL);
	assert(m_pDevice == NULL);
	assert(m_pImmediateContext == NULL);
	assert(m_pPointerShapeBuffer == NULL && m_iPointerShapeBufferSize == 0);
	assert(m_pMetadataBuffer == NULL && m_iMetadataBufferSize == 0);
	assert(m_pMoveRectsBegin == NULL && m_iMoveRectsCount == 0);
	assert(m_pDirtyRectsBegin == NULL && m_iDirtyRectsCount == 0);
}