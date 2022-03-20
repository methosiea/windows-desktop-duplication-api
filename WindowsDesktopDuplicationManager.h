#pragma once

#include <dxgi1_2.h>
#include <d3d11.h>

class WindowsDesktopDuplicationManager
{
public:
	WindowsDesktopDuplicationManager();
	~WindowsDesktopDuplicationManager();

	void Update();
	void Release();

private:
	IDXGIFactory1* m_pFactory = NULL;
	IDXGIAdapter1* m_pPrimaryAdapter = NULL;
	IDXGIOutput1* m_pPrimaryOutput = NULL;
	IDXGIOutputDuplication* m_pOutputDuplication = NULL;
	DXGI_OUTDUPL_DESC m_outputDuplicationDesc = {};
	ID3D11Device* m_pDevice = NULL;
	ID3D11DeviceContext* m_pImmediateContext = NULL;
	DXGI_OUTDUPL_FRAME_INFO m_frameInfo = {};
	IDXGIResource* m_pDesktopResource = NULL;

	BYTE* m_pBitmapDataBuffer = NULL;
	UINT m_iBitmapDataBufferSize = 0;
	UINT m_iBitmapDataBufferSizeRequired = 0;

	BYTE* m_pPointerShapeBuffer = NULL;
	UINT m_iPointerShapeBufferSize = 0;
	UINT m_iPointerShapeBufferSizeRequired = 0;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO m_pointerShapeInfo = {};

	BYTE* m_pMetadataBuffer = NULL;
	UINT m_iMetadataBufferSize = 0;

	DXGI_OUTDUPL_MOVE_RECT* m_pMoveRectsBegin = NULL;
	UINT m_iMoveRectsCount = 0;

	RECT* m_pDirtyRectsBegin = NULL;
	UINT m_iDirtyRectsCount = 0;

	BOOL m_bLastPointerVisibility = TRUE;

	void InitializeOutputDuplication();
	void ReleaseOutputDuplication();

	void ReleasePointerShapeBuffer();
	void ReleaseMetadataBuffer();
	void ReleaseBitmapDataBuffer();

	void OnPointerVisibilityChanged();
	void OnPointerShapeChanged();
	void OnDesktopImageChanged();

	void UpdatePointerShapeBuffer();
	void UpdateMetadataBuffer();
	void UpdateBitmapDataBuffer();
};

