#pragma once
#include <vector>

#include <mfidl.h>
#pragma comment(lib, "mf")

#include <mfreadwrite.h>
#pragma comment(lib, "mfreadwrite")

#include <wrl/client.h>
#include <comdef.h>

#ifdef __IMFMediaSource_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFMediaSource, IID_IMFMediaSource);
#endif // __IMFMediaSource_INTERFACE_DEFINED__

#ifdef __IMFStreamDescriptor_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFStreamDescriptor, IID_IMFStreamDescriptor);
#endif // __IMFStreamDescriptor_INTERFACE_DEFINED__

#ifdef __IMFMediaTypeHandler_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFMediaTypeHandler, IID_IMFMediaTypeHandler);
#endif // __IMFMediaTypeHandler_INTERFACE_DEFINED__

#ifdef __IMFMediaType_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFMediaType, IID_IMFMediaType);
#endif // __IMFMediaType_INTERFACE_DEFINED__

#ifdef __IMFSourceReader_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFSourceReader, IID_IMFSourceReader);
#endif // __IMFSourceReader_INTERFACE_DEFINED__

#ifdef __IMFSourceReader_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFSourceReaderEx, IID_IMFSourceReaderEx);
#endif // __IMFSourceReader_INTERFACE_DEFINED__

#ifdef __IMFSample_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFSample, IID_IMFSample);
#endif // __IMFSample_INTERFACE_DEFINED__

#ifdef __IMFTransform_INTERFACE_DEFINED__
_COM_SMARTPTR_TYPEDEF(IMFTransform, IID_IMFTransform);
#endif // __IMFTransform_INTERFACE_DEFINED__

bool MediaInitialize();
bool MediaUninitialize();

class MediaDeviceSet;
class MediaReader;
class MediaReader2;

class MediaDeviceSet
{
public:
	MediaDeviceSet();
	~MediaDeviceSet();
	explicit operator bool() const;

	bool CreatVideoSet();

	UINT32 GetDeviceCount() const;
	bool GetDeviceName(UINT32 DeviceIndex, WCHAR *&pDeviceName) const;
	bool GetMediaReader(UINT32 DeviceIndex, MediaReader &MediaReader);

private:
	UINT32 DeviceCount;
	IMFActivate **ppDevices;

	WCHAR **ppDeviceName;

	HRESULT EnumMediaDevice(GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID);
	HRESULT EnumDeviceName();
};

class MediaReader2
{
public:
	MediaReader2();
	~MediaReader2();
	explicit operator bool() const;

	bool CreatFromMediaSource(IMFMediaSourcePtr MediaSourcePtr, bool VideoProcessing);

	bool GetCurrentFrameSize(DWORD StreamIndex, UINT32 *pWidth, UINT32 *pHeight);
	bool GetCurrentFrameRate(DWORD StreamIndex, UINT32 *pNumerator, UINT32 *pDenominator);
	bool GetCurrenBitmapImage(DWORD StreamIndex, BYTE *pBuffer, bool *pNewImage);

	bool ConvertYUVVideoToRGB32(DWORD StreamIndex);

private:
	Microsoft::WRL::ComPtr<IMFSourceReader> SourceReaderPtr;
	IMFSourceReaderExPtr ReaderExPtr;
	DWORD StreamCount;
	std::vector<IMFMediaTypePtr> vTypePtr;

	HRESULT ReadSample(DWORD StreamIndex, IMFSample **ppSample);
};

class MediaSource : public IMFMediaSourcePtr
{
public:
	MediaSource();
	~MediaSource();

	bool Creat(IMFMediaSourcePtr MediaSourcePtr);

	DWORD GetStreamCount() const;
	bool GetMediaTypeHandler(DWORD StreamIndex, IMFMediaTypeHandler **ppMediaTypeHandler) const;

private:
	DWORD StreamCount;
	std::vector<IMFStreamDescriptorPtr> ppDescriptor;

	HRESULT EnumStream(IMFMediaSource *pMediaSource);
};

class MediaTypeHandler : public IMFMediaTypeHandlerPtr
{
public:
	MediaTypeHandler();
	~MediaTypeHandler();

	bool Creat(MediaSource &Source, DWORD StreamIndex);

	DWORD GetTypeCount() const;
	bool GetMediaType(DWORD TypeIndex, IMFMediaType **ppMediaType);
	bool GetCurrentMediaType(IMFMediaType **ppMediaType);

private:
	DWORD TypeCount;

	HRESULT EnumType(IMFMediaTypeHandler *pMediaTypeHandler);
};

class MediaType : public IMFMediaTypePtr
{
public:
	MediaType();
	~MediaType();

	bool Creat(IMFMediaTypePtr MediaTypePtr);
	bool CopyFrom(MediaType &MediaType);

	bool CreatCurrent(MediaTypeHandler &TypeHandler);

	bool SetSubTypeToRGB24();

	UINT32 GetWidth() const;
	UINT32 GetHeight() const;
	UINT32 GetDenominator() const;
	UINT32 GetNumerator() const;

	GUID GetMajorType() const;
	GUID GetSubType() const;

private:
	UINT32 Height;
	UINT32 Width;
	UINT32 Denominator;
	UINT32 Numerator;

	GUID MajorType;
	GUID SubType;

	HRESULT EnumFormat(IMFMediaType *pMediaType);
};

class Sample : public IMFSamplePtr
{
public:
	Sample();
	~Sample();

	bool Creat(IMFSamplePtr SamplePtr);
	bool CreatFromMediaType(MediaType &MediaType);

	bool Convert(MediaType &MediaType, BYTE *pBuffer);

private:
};

class SourceReader : public IMFSourceReaderPtr
{
public:
	SourceReader();
	~SourceReader();

	bool CreatFromMediaSource(MediaSource &Source);

	bool GetSample(DWORD StreamIndex, Sample &Sample);

private:
};

class Transform : public IMFTransformPtr
{
public:
	Transform();
	~Transform();

	bool SetType(MediaType &InputType, MediaType &OutputType);

	bool Process(Sample &InputSample, Sample &OutputSample);

private:
	MFT_OUTPUT_DATA_BUFFER OutputSamples;
};

class MediaReader
{
public:
	MediaReader();
	~MediaReader();
	explicit operator bool() const;

	bool Creat(IMFMediaSourcePtr MediaSourcePtr, bool Default = true);
	bool SetStream(DWORD StreamIndex);

	bool GetCurrentFrameSize(DWORD StreamIndex, UINT32 *pWidth, UINT32 *pHeight);
	bool GetCurrenBitmapImage(DWORD StreamIndex, BYTE *pBuffer, bool *pNewImage);

private:
	bool Created;

	SourceReader m_Reader;
	Transform m_Transform;

	MediaType m_InputType;
	MediaType m_OutputType;

	Sample m_InputSample;
	Sample m_OutputSample;

	MediaSource m_Source;
	DWORD m_StreamCount;

	std::vector<MediaTypeHandler> m_pTypeHandler;
	std::vector<DWORD> m_pTypeCount;
};
