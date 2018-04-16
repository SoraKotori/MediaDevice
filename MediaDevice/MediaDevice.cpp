#include "MediaDevice.h"

#include <mfapi.h>
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#include <Mferror.h>

DEFINE_GUID(CLSID_VideoProcessorMFT, 0x88753b26, 0x5b24, 0x49bd, 0xb2, 0xe7, 0xc, 0x44, 0x5c, 0x78, 0xc9, 0x82);

#ifdef __IMFPresentationDescriptor_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFPresentationDescriptor, IID_IMFPresentationDescriptor);
#endif // __IMFPresentationDescriptor_INTERFACE_DEFINED__

#ifdef __IMFAttributes_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFAttributes, IID_IMFAttributes);
#endif // __IMFAttributes_INTERFACE_DEFINED__

#ifdef __IMFMediaBuffer_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFMediaBuffer, IID_IMFMediaBuffer);
#endif // __IMFMediaBuffer_INTERFACE_DEFINED__

#ifdef __IMF2DBuffer2_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMF2DBuffer, IID_IMF2DBuffer);
#endif // __IMF2DBuffer2_INTERFACE_DEFINED__

#ifdef __IMF2DBuffer2_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMF2DBuffer2, IID_IMF2DBuffer2);
#endif // __IMF2DBuffer2_INTERFACE_DEFINED__

using namespace std;

bool MediaInitialize()
{
	HRESULT hResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = MFStartup(MF_VERSION);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

bool MediaUninitialize()
{
	HRESULT hResult = MFShutdown();
	if (FAILED(hResult))
	{
		return false;
	}

	CoUninitialize();
	return true;
}

MediaDeviceSet::MediaDeviceSet() :
	DeviceCount(0U)
{
}

MediaDeviceSet::~MediaDeviceSet()
{
	if (0U != DeviceCount)
	{
		for (UINT32 Index = 0U; Index < DeviceCount; Index++)
		{
			ppDevices[Index]->Release();
			CoTaskMemFree(ppDeviceName[Index]);
		}

		CoTaskMemFree(ppDevices);
		CoTaskMemFree(ppDeviceName);

		DeviceCount = 0U;
	}
}

MediaDeviceSet::operator bool() const
{
	return 0U != DeviceCount;
}

bool MediaDeviceSet::CreatVideoSet()
{
	HRESULT hResult = EnumMediaDevice(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = EnumDeviceName();
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

HRESULT MediaDeviceSet::EnumMediaDevice(GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID)
{
	IMFAttributesPtr AttributesPtr = nullptr;
	HRESULT hResult = MFCreateAttributes(&AttributesPtr, 1);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = AttributesPtr->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID
	);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFActivate **ppActivate = nullptr;
	UINT32  ActivateCount = 0;
	hResult = MFEnumDeviceSources(AttributesPtr, &ppActivate, &ActivateCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	if (0 == ActivateCount)
	{
		return MF_E_NOT_FOUND;
	}

	DeviceCount = ActivateCount;
	ppDevices = ppActivate;
	return S_OK;
};

HRESULT MediaDeviceSet::EnumDeviceName()
{
	LPVOID CoTaskMemory = CoTaskMemAlloc(sizeof(WCHAR*) * DeviceCount);
	if (nullptr == CoTaskMemory)
	{
		return S_FALSE;
	}

	WCHAR **ppMediaDeviceName = reinterpret_cast<WCHAR**>(CoTaskMemory);
	for (UINT32 Index = 0; Index < DeviceCount; Index++)
	{
		LPWSTR pwszValue = nullptr;
		UINT32 cchLength = 0;

		HRESULT hResult = ppDevices[Index]->GetAllocatedString(
			MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
			&pwszValue,
			&cchLength
		);
		if (FAILED(hResult))
		{
			return hResult;
		}

		ppMediaDeviceName[Index] = pwszValue;
	}

	ppDeviceName = ppMediaDeviceName;
	return S_OK;
	//unique_ptr<WCHAR, void(__stdcall*)(LPVOID)> up(pwszValue, CoTaskMemFree);

	//pDeviceName[Index] = shared_ptr<WCHAR>(pwszValue, CoTaskMemFree);

	//pDeviceName[Index] = shared_ptr<WCHAR>(pwszValue, [](WCHAR *pName) { CoTaskMemFree(pName); });
}

UINT32 MediaDeviceSet::GetDeviceCount() const
{
	return DeviceCount;
}

bool MediaDeviceSet::GetDeviceName(UINT32 DeviceIndex, WCHAR *&pDeviceName) const
{
	if (DeviceCount <= DeviceIndex)
	{
		return false;
	}

	pDeviceName = ppDeviceName[DeviceIndex];
	return true;
}

bool MediaDeviceSet::GetMediaReader(UINT32 DeviceIndex, MediaReader &MediaReader)
{
	if (DeviceCount <= DeviceIndex)
	{
		return false;
	}

	IMFMediaSourcePtr MediaSourcePtr;
	HRESULT hResult = ppDevices[DeviceIndex]->ActivateObject(IID_PPV_ARGS(&MediaSourcePtr));
	if (FAILED(hResult))
	{
		return false;
	}

	bool bResult = MediaReader.Creat(MediaSourcePtr, true);
	if (false == bResult)
	{
		return false;
	}

	return true;
}

MediaSource::MediaSource()
{
}

MediaSource::~MediaSource()
{
	if (true == (*this).operator bool())
	{
		(*this)->AddRef();
		ULONG RefCount = (*this)->Release();
		if (0 == RefCount - 1)
		{
			HRESULT hResult = (*this)->Shutdown();
		}
	}
}

bool MediaSource::Creat(IMFMediaSourcePtr MediaSourcePtr)
{
	HRESULT hResult = EnumStream(MediaSourcePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFMediaSourcePtr::operator=(MediaSourcePtr);
	return true;
}

HRESULT MediaSource::EnumStream(IMFMediaSource *pMediaSource)
{
	IMFPresentationDescriptorPtr pPresentationDescriptor = nullptr;
	HRESULT hResult = pMediaSource->CreatePresentationDescriptor(&pPresentationDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD DescriptorCount = 0UL;
	hResult = pPresentationDescriptor->GetStreamDescriptorCount(&DescriptorCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto ppStreamDescriptor = vector<IMFStreamDescriptorPtr>(DescriptorCount);
	for (UINT32 Index = 0; Index < DescriptorCount; Index++)
	{
		BOOL fSelected = FALSE;
		IMFStreamDescriptorPtr pStreamDescriptor = nullptr;

		hResult = pPresentationDescriptor->GetStreamDescriptorByIndex(Index, &fSelected, &pStreamDescriptor);
		if (FAILED(hResult))
		{
			return hResult;
		}

		ppStreamDescriptor[Index] = pStreamDescriptor;
	}

	ppDescriptor = ppStreamDescriptor;
	StreamCount = DescriptorCount;
	return S_OK;
}

DWORD MediaSource::GetStreamCount() const
{
	return StreamCount;
}

bool MediaSource::GetMediaTypeHandler(DWORD StreamIndex, IMFMediaTypeHandler **ppMediaTypeHandler) const
{
	if (StreamCount <= StreamIndex)
	{
		return false;
	}

	HRESULT hResult = ppDescriptor[StreamIndex]->GetMediaTypeHandler(ppMediaTypeHandler);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

MediaTypeHandler::MediaTypeHandler()
{
}

MediaTypeHandler::~MediaTypeHandler()
{
}

bool MediaTypeHandler::Creat(MediaSource &Source, DWORD StreamIndex)
{
	bool bResult = Source.operator bool();
	if (false == bResult)
	{
		return false;
	}
	MediaTypeHandler::~MediaTypeHandler();

	IMFMediaTypeHandlerPtr pMediaTypeHandler = nullptr;
	bResult = Source.GetMediaTypeHandler(StreamIndex, &pMediaTypeHandler);
	if (false == bResult)
	{
		return false;
	}

	HRESULT hResult = EnumType(pMediaTypeHandler);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFMediaTypeHandlerPtr::operator=(pMediaTypeHandler);
	return true;
}

DWORD MediaTypeHandler::GetTypeCount() const
{
	return TypeCount;
}

bool MediaTypeHandler::GetMediaType(DWORD TypeIndex, IMFMediaType ** ppMediaType)
{
	return true;
}

bool MediaTypeHandler::GetCurrentMediaType(IMFMediaType **ppMediaType)
{
	HRESULT hResult = (*this)->GetCurrentMediaType(ppMediaType);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

HRESULT MediaTypeHandler::EnumType(IMFMediaTypeHandler *pMediaTypeHandler)
{
	DWORD dwTypeCount = 0UL;
	HRESULT hResult = pMediaTypeHandler->GetMediaTypeCount(&dwTypeCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	TypeCount = dwTypeCount;
	return S_OK;
}

MediaType::MediaType()
{
}

MediaType::~MediaType()
{
}

bool MediaType::Creat(IMFMediaTypePtr MediaTypePtr)
{
	HRESULT hResult = EnumFormat(MediaTypePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFMediaTypePtr::operator=(MediaTypePtr);
	return true;
}

bool MediaType::CopyFrom(MediaType &MediaType)
{
	IMFMediaTypePtr MediaTypePtr = nullptr;
	HRESULT hResult = MFCreateMediaType(&MediaTypePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = MediaType->CopyAllItems(MediaTypePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	bool bResult = Creat(MediaTypePtr);
	if (false == bResult)
	{
		return false;
	}

	return true;
}

bool MediaType::CreatCurrent(MediaTypeHandler &TypeHandler)
{
	bool bResult = TypeHandler;
	if (false == bResult)
	{
		return false;
	}

	IMFMediaTypePtr pMediaType = nullptr;
	bResult = TypeHandler.GetCurrentMediaType(&pMediaType);
	if (false == bResult)
	{
		return false;
	}

	HRESULT hResult = EnumFormat(pMediaType);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFMediaTypePtr::operator=(pMediaType);
	return true;
}

bool MediaType::SetSubTypeToRGB24()
{
	HRESULT hResult = (*this)->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB24);
	if (FAILED(hResult))
	{
		return false;
	}

	SubType = MFVideoFormat_RGB24;
	return true;
}

UINT32 MediaType::GetWidth() const
{
	return Width;
}

UINT32 MediaType::GetHeight() const
{
	return Height;
}

UINT32 MediaType::GetDenominator() const
{
	return Denominator;
}

UINT32 MediaType::GetNumerator() const
{
	return Numerator;
}

GUID MediaType::GetMajorType() const
{
	return MajorType;
}

GUID MediaType::GetSubType() const
{
	return SubType;
}

HRESULT MediaType::EnumFormat(IMFMediaType *pMediaType)
{
	UINT64 FrameSize = 0ULL;
	HRESULT hResult = pMediaType->GetUINT64(MF_MT_FRAME_SIZE, &FrameSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 FrameRate = 0ULL;
	hResult = pMediaType->GetUINT64(MF_MT_FRAME_RATE, &FrameRate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID guidMajorType = { 0 };
	hResult = pMediaType->GetGUID(MF_MT_MAJOR_TYPE, &guidMajorType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID guidSubType = { 0 };
	hResult = pMediaType->GetGUID(MF_MT_SUBTYPE, &guidSubType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	Height = LO32(FrameSize);
	Width = HI32(FrameSize);
	Denominator = LO32(FrameRate);
	Numerator = HI32(FrameRate);

	MajorType = guidMajorType;
	SubType = guidSubType;

	return S_OK;
}

SourceReader::SourceReader()
{
}

SourceReader::~SourceReader()
{
}

bool SourceReader::CreatFromMediaSource(MediaSource &Source)
{
	IMFSourceReaderPtr SourceReaderPtr = nullptr;
	HRESULT hResult = MFCreateSourceReaderFromMediaSource(Source, nullptr, &SourceReaderPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFSourceReaderPtr::operator=(SourceReaderPtr);
	return true;
}

bool SourceReader::GetSample(DWORD StreamIndex, Sample &Sample)
{
	DWORD ActualStreamIndex = 0UL;
	DWORD StreamFlags = 0UL;
	LONGLONG Timestamp = 0LL;
	IMFSamplePtr SamplePtr = nullptr;

	HRESULT hResult = (*this)->ReadSample(
		StreamIndex,
		0,
		&ActualStreamIndex,
		&StreamFlags,
		&Timestamp,
		&SamplePtr
	);
	if (FAILED(hResult))
	{
		return false;
	}
	0xc00d36b4; E_INVALIDARG;
	if (StreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
	{
		return false;
	}

	bool bResult = Sample.Creat(SamplePtr);
	if (false == bResult)
	{
		return false;
	}

	return true;
}

Sample::Sample()
{
}

Sample::~Sample()
{
}

bool Sample::Creat(IMFSamplePtr SamplePtr)
{
	IMFSamplePtr::operator=(SamplePtr);
	return true;
}

bool Sample::CreatFromMediaType(MediaType &MediaType)
{
	IMFSamplePtr SamplePtr = nullptr;
	HRESULT hResult = MFCreateSample(&SamplePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFMediaBufferPtr BufferPtr = nullptr;
	hResult = MFCreateMediaBufferFromMediaType(MediaType, 0LL, 0UL, 0UL, &BufferPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = SamplePtr->AddBuffer(BufferPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	bool bResult = Creat(SamplePtr);
	if (false == bResult)
	{
		false;
	}

	return true;
}

bool Sample::Convert(MediaType &MediaType, BYTE *pBuffer)
{
	IMFMediaBufferPtr MediaBufferPtr = nullptr;
	HRESULT hResult = (*this)->ConvertToContiguousBuffer(&MediaBufferPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMF2DBuffer2Ptr _2DBuffer2Ptr = nullptr;
	hResult = MediaBufferPtr->QueryInterface(&_2DBuffer2Ptr);
	if (FAILED(hResult))
	{
		return false;
	}

	BYTE *pbScanline0;
	LONG lPitch;
	BYTE *pbBufferStart;
	DWORD cbBufferLength;

	hResult = _2DBuffer2Ptr->Lock2DSize(
		MF2DBuffer_LockFlags_Read,
		&pbScanline0,
		&lPitch,
		&pbBufferStart,
		&cbBufferLength
	);
	if (FAILED(hResult))
	{
		return false;
	}

	GUID SubType = MediaType.GetSubType();
	UINT32 Width = MediaType.GetWidth();
	UINT32 Height = MediaType.GetHeight();

	LONG Stride = 0L;
	hResult = MFGetStrideForBitmapInfoHeader(SubType.Data1, Width, &Stride);
	if (FAILED(hResult))
	{
		return false;
	}

	//DWORD ImageSize = 0U;
	//hResult = MFGetPlaneSize(SubType.Data1, Width, Height, &ImageSize);
	//if (FAILED(hResult))
	//{
	//	return false;
	//}

	if (Stride < 0)
	{
		pBuffer += (-Stride) * (Height - 1);
	}

	UINT32 WidthByte = (lPitch / Width) * Width;
	hResult = MFCopyImage(pBuffer, Stride, pbBufferStart, lPitch, WidthByte, Height);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = _2DBuffer2Ptr->Unlock2D();
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

Transform::Transform() :
	OutputSamples({ 0UL, nullptr, 0UL, nullptr })
{
	auto Result = [this]()->bool
	{
		IMFTransformPtr TransformPtr = nullptr;
		HRESULT hResult = CoCreateInstance(CLSID_VideoProcessorMFT, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&TransformPtr));
		if (FAILED(hResult))
		{
			return false;
		}

		IMFAttributesPtr AttributesPtr = nullptr;
		hResult = (*this)->GetAttributes(&AttributesPtr);
		if (FAILED(hResult))
		{
			return false;
		}

		BOOL BResult = MFGetAttributeUINT32(AttributesPtr, MF_SA_D3D_AWARE, FALSE);
		//MFT_INPUT_STREAM_INFO StreamInfo =
		//{
		//	0LL,
		//	MFT_INPUT_STREAM_DOES_NOT_ADDREF,
		//	0UL,
		//	0UL,
		//	0UL
		//};

		//hResult = TransformPtr->GetInputStreamInfo(0UL, &StreamInfo);
		//if (FAILED(hResult))
		//{
		//	return false;
		//}

		IMFTransformPtr::operator=(TransformPtr);
		return true;
	}();
}

Transform::~Transform()
{
}

bool Transform::SetType(MediaType &InputType, MediaType &OutputType)
{
	HRESULT hResult = (*this)->SetInputType(0UL, InputType, 0UL);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = (*this)->SetOutputType(0UL, OutputType, 0UL);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

bool Transform::Process(Sample &InputSample, Sample &OutputSample)
{
	HRESULT hResult = (*this)->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0UL);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = (*this)->ProcessInput(0UL, InputSample, 0UL);
	if (FAILED(hResult))
	{
		return false;
	}

	OutputSamples.pSample = OutputSample;
	DWORD Status = 0UL;
	hResult = (*this)->ProcessOutput(0UL, 1UL, &OutputSamples, &Status);
	if (FAILED(hResult))
	{
		return false;
	}

	//if (MF_E_TRANSFORM_NEED_MORE_INPUT != hResult)
	//{
	//	return false;
	//}

	return true;
}

MediaReader::MediaReader() :
	Created(false)
{
}

MediaReader::~MediaReader()
{
	Created = false;
}

MediaReader::operator bool() const
{
	return Created;
}

bool MediaReader::Creat(IMFMediaSourcePtr MediaSourcePtr, bool Default)
{
	MediaSource Source;
	bool bResult = Source.Creat(MediaSourcePtr);
	if (false == bResult)
	{
		return false;
	}

	SourceReader Reader;
	bResult = Reader.CreatFromMediaSource(Source);
	if (false == bResult)
	{
		return false;
	}

	DWORD StreamCount = Source.GetStreamCount();
	vector<MediaTypeHandler> pTypeHandler(StreamCount);
	for (DWORD Index = 0; Index < StreamCount; Index++)
	{
		MediaTypeHandler TypeHandler;
		bResult = TypeHandler.Creat(Source, Index);
		if (false == bResult)
		{
			return false;
		}

		pTypeHandler[Index] = TypeHandler;
	}

	MediaType InputType;
	bResult = InputType.CreatCurrent(pTypeHandler[0]);
	if (false == bResult)
	{
		return false;
	}

	MediaType OutputType;
	bResult = OutputType.CopyFrom(InputType);
	if (false == bResult)
	{
		return false;
	}

	bResult = OutputType.SetSubTypeToRGB24();
	if (false == bResult)
	{
		return false;
	}

	Transform Transform;
	bResult = Transform.SetType(InputType, OutputType);
	if (false == bResult)
	{
		return false;
	}

	Sample InputSample;
	bResult = InputSample.CreatFromMediaType(InputType);
	if (false == bResult)
	{
		return false;
	}

	Sample OutputSample;
	bResult = OutputSample.CreatFromMediaType(OutputType);
	if (false == bResult)
	{
		return false;
	}

	m_Reader = Reader;
	m_Transform = Transform;

	m_InputType = InputType;
	m_OutputType = OutputType;

	m_InputSample = InputSample;
	m_OutputSample = OutputSample;

	m_Source = Source;
	m_StreamCount = StreamCount;

	m_pTypeHandler;
	m_pTypeCount;

	Created = true;
	return true;
}

bool MediaReader::GetCurrentFrameSize(DWORD StreamIndex, UINT32 * pWidth, UINT32 * pHeight)
{
	*pWidth = m_OutputType.GetWidth();
	*pHeight = m_OutputType.GetHeight();
	return true;
}

bool MediaReader::GetCurrenBitmapImage(DWORD StreamIndex, BYTE * pBuffer, bool *pNewImage)
{
	if (false == Created)
	{
		return false;
	}

	Sample InputSample;
	bool bResult = m_Reader.GetSample(StreamIndex, InputSample);
	if (false == bResult)
	{
		return false;
	}

	if (nullptr == InputSample)
	{
		*pNewImage = false;
		return true;
	}

	bResult = m_Transform.Process(InputSample, m_OutputSample);
	if (false == bResult)
	{
		return false;
	}

	bResult = m_OutputSample.Convert(m_OutputType, pBuffer);
	if (false == bResult)
	{
		return false;
	}

	*pNewImage = true;
	return true;
}

MediaReader2::MediaReader2()
{
}

MediaReader2::~MediaReader2()
{
}

MediaReader2::operator bool() const
{
	return ReaderExPtr;
}

bool MediaReader2::CreatFromMediaSource(IMFMediaSourcePtr MediaSourcePtr, bool VideoProcessing)
{
	IMFAttributesPtr AttributesPtr = nullptr;
	if (true == VideoProcessing)
	{
		HRESULT hResult = MFCreateAttributes(&AttributesPtr, 1U);
		if (FAILED(hResult))
		{
			return false;
		}

		hResult = AttributesPtr->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
		if (FAILED(hResult))
		{
			return false;
		}
	}

	IMFSourceReaderPtr SourceReaderPtr = nullptr;
	HRESULT hResult = MFCreateSourceReaderFromMediaSource(MediaSourcePtr, AttributesPtr, &SourceReaderPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMFSourceReaderExPtr SourceReaderExPtr = nullptr;
	hResult = SourceReaderPtr->QueryInterface(IID_PPV_ARGS(&SourceReaderExPtr));
	if (FAILED(hResult))
	{
		return false;
	}

	DWORD _StreamCount = 0UL;
	vector<IMFMediaTypePtr> vMediaTypePtr;
	while (true)
	{
		IMFMediaTypePtr MediaTypePtr = nullptr;
		hResult = SourceReaderExPtr->GetCurrentMediaType(_StreamCount, &MediaTypePtr);
		if (MF_E_INVALIDSTREAMNUMBER == hResult)
		{
			break;
		}
		else if (FAILED(hResult))
		{
			return false;
		}

		_StreamCount++;
		vMediaTypePtr.push_back(MediaTypePtr);
	}

	vTypePtr = vMediaTypePtr;
	StreamCount = _StreamCount;
	ReaderExPtr = SourceReaderExPtr;
	return true;
}

bool MediaReader2::GetCurrentFrameSize(DWORD StreamIndex, UINT32 *pWidth, UINT32 *pHeight)
{
	if (StreamCount <= StreamIndex)
	{
		return false;
	}

	HRESULT hResult = MFGetAttributeSize(vTypePtr[StreamIndex], MF_MT_FRAME_SIZE, pWidth, pHeight);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

bool MediaReader2::GetCurrentFrameRate(DWORD StreamIndex, UINT32 *pNumerator, UINT32 *pDenominator)
{
	if (StreamCount <= StreamIndex)
	{
		return false;
	}

	HRESULT hResult = MFGetAttributeRatio(vTypePtr[StreamIndex], MF_MT_FRAME_RATE, pNumerator, pDenominator);
	if (FAILED(hResult))
	{
		return false;
	}

	return true;
}

bool MediaReader2::GetCurrenBitmapImage(DWORD StreamIndex, BYTE *pBuffer, bool *pNewImage)
{
	IMFSamplePtr SamplePtr = nullptr;
	HRESULT hResult = ReadSample(StreamIndex, &SamplePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	if (nullptr == SamplePtr)
	{
		*pNewImage = false;
		return true;
	}

	IMFMediaBufferPtr MediaBufferPtr = nullptr;
	hResult = SamplePtr->ConvertToContiguousBuffer(&MediaBufferPtr);
	if (FAILED(hResult))
	{
		return false;
	}

	IMF2DBuffer2Ptr _2DBuffer2Ptr = nullptr;
	hResult = MediaBufferPtr->QueryInterface(&_2DBuffer2Ptr);
	if (FAILED(hResult))
	{
		return false;
	}

	BYTE *pbScanline0;
	LONG lPitch;
	BYTE *pbBufferStart;
	DWORD cbBufferLength;

	hResult = _2DBuffer2Ptr->Lock2DSize(
		MF2DBuffer_LockFlags_Read,
		&pbScanline0,
		&lPitch,
		&pbBufferStart,
		&cbBufferLength
	);
	if (FAILED(hResult))
	{
		return false;
	}

	UINT32 Width = 0U;
	UINT32 Height = 0U;
	bool bResult = GetCurrentFrameSize(StreamIndex, &Width, &Height);
	if (false == bResult)
	{
		return false;
	}

	LONG Stride = 0L;
	hResult = MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB24.Data1, Width, &Stride);
	if (FAILED(hResult))
	{
		return false;
	}

	if (Stride < 0)
	{
		pBuffer += (-Stride) * (Height - 1);
	}

	UINT32 WidthByte = (lPitch / Width) * Width;
	hResult = MFCopyImage(pBuffer, Stride, pbBufferStart, lPitch, WidthByte, Height);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = _2DBuffer2Ptr->Unlock2D();
	if (FAILED(hResult))
	{
		return false;
	}

	*pNewImage = true;
	return true;
}

bool MediaReader2::ConvertYUVVideoToRGB32(DWORD StreamIndex)
{
	if (StreamCount <= StreamIndex)
	{
		return false;
	}

	IMFMediaTypePtr RGB32TypePtr = nullptr;
	HRESULT hResult = MFCreateMediaType(&RGB32TypePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = RGB32TypePtr->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = RGB32TypePtr->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	if (FAILED(hResult))
	{
		return false;
	}

	hResult = ReaderExPtr->SetCurrentMediaType(StreamIndex, 0UL, RGB32TypePtr);
	if (FAILED(hResult))
	{
		if (MF_E_TOPO_CODEC_NOT_FOUND != hResult)
		{
			return false;
		}
		else
		{
			return false;
		}
	}

	IMFMediaTypePtr NewTypePtr = nullptr;
	hResult = ReaderExPtr->GetCurrentMediaType(StreamIndex, &NewTypePtr);
	if (FAILED(hResult))
	{
		return false;
	}

	vTypePtr[StreamIndex] = NewTypePtr;
	return true;
}

HRESULT MediaReader2::ReadSample(DWORD StreamIndex, IMFSample **ppSample)
{
	DWORD ActualStreamIndex = 0UL;
	DWORD StreamFlags = 0UL;
	LONGLONG Timestamp = 0LL;
	IMFSample *pSample = nullptr;

	HRESULT hResult = ReaderExPtr->ReadSample(
		StreamIndex,
		0,
		&ActualStreamIndex,
		&StreamFlags,
		&Timestamp,
		&pSample
	);
	if (FAILED(hResult))
	{
		return hResult;
	}

	*ppSample = pSample;
	return S_OK;
}
