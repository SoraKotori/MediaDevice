#include "Media Foundation.h"

#include <mfapi.h>
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#include <mfidl.h>

#include <Mferror.h>

#include <wrl/client.h>
#include <memory>

using namespace std;
using namespace Microsoft::WRL;

class MediaDeviceSourceSet
{
public:
	MediaDeviceSourceSet();
	~MediaDeviceSourceSet();

	HRESULT EnumAudioDeviceSources(UINT32* pSourceCount);
	HRESULT EnumVideoDeviceSources(UINT32* pSourceCount);

	HRESULT GetFriendlyName(UINT32 SourceIndex, WCHAR **ppFriendlyName);
	HRESULT GetMediaType(UINT32 SourceIndex, MFT_REGISTER_TYPE_INFO *pMediaType);
	HRESULT GetSourceType(UINT32 SourceIndex, GUID *pSourceType);
	HRESULT GetSourceTypeAudcapEndpointID(UINT32 SourceIndex, WCHAR **ppSourceTypeAudcapEndpointID);
	HRESULT GetSourceTypeVidcapCategory(UINT32 SourceIndex, GUID *pSourceTypeVidcapCategory);
	HRESULT GetSourceTypeVidcapHWSource(UINT32 SourceIndex, UINT32 *pSourceTypeVidcapHWSource);
	HRESULT GetSourceTypeVidcapSymbolicLink(UINT32 SourceIndex, WCHAR **ppSourceTypeVidcapSymbolicLink);

private:
	typedef struct MF_DEVSOURCE_ATTRIBUTE
	{
		unique_ptr<WCHAR[]>		FriendlyName;
		MFT_REGISTER_TYPE_INFO	MediaType;
		GUID					SourceType;
		unique_ptr<WCHAR[]>		SourceTypeAudcapEndpointID;
		GUID					SourceTypeVidcapCategory;
		UINT32					SourceTypeVidcapHWSource;
		unique_ptr<WCHAR[]>		SourceTypeVidcapSymbolicLink;
	}MF_DEVSOURCE_ATTRIBUTE;

	unique_ptr<MF_DEVSOURCE_ATTRIBUTE[]>	m_pDevsourceAttribute;
	IMFActivate**							m_ppSourceActivate;
	UINT32									m_cSourceActivate;

	HRESULT EnumMediaDeviceSources(GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID, IMFActivate ***pppSourceActivate, UINT32 *pcSourceActivate);
	HRESULT GetMediaDeviceSourceAttribute(IMFActivate *pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE *pDevsourceAttributeIndex);
	HRESULT GetVideoDeviceSourceAttribute(IMFActivate *pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE *pDevsourceAttributeIndex);
	HRESULT GetAudioDeviceSourceAttribute(IMFActivate *pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE *pDevsourceAttributeIndex);
};

MediaDeviceSourceSet::MediaDeviceSourceSet() :
	m_pDevsourceAttribute(nullptr),
	m_ppSourceActivate(nullptr),
	m_cSourceActivate(0U)
{
}

MediaDeviceSourceSet::~MediaDeviceSourceSet()
{
	if (0U != m_cSourceActivate)
	{
		for (UINT32 Index = 0U; Index < m_cSourceActivate; Index++)
		{
			m_ppSourceActivate[Index]->Release();
		}

		CoTaskMemFree(m_ppSourceActivate);

		m_ppSourceActivate = nullptr;
		m_cSourceActivate = 0U;
	}
}

HRESULT MediaDeviceSourceSet::EnumAudioDeviceSources(UINT32 *pSourceCount)
{
	IMFActivate **ppSourceActivate = nullptr;
	UINT32 cSourceActivate = 0U;

	HRESULT hResult = EnumMediaDeviceSources(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID, &ppSourceActivate, &cSourceActivate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto pDevsourceAttribute = make_unique<struct MF_DEVSOURCE_ATTRIBUTE[]>(cSourceActivate);
	for (UINT32 Index = 0; Index < cSourceActivate; Index++)
	{
		IMFActivate *pSourceActivateIndex = ppSourceActivate[Index];
		auto pDevsourceAttributeIndex = &pDevsourceAttribute[Index];

		hResult = GetMediaDeviceSourceAttribute(pSourceActivateIndex, pDevsourceAttributeIndex);
		if (FAILED(hResult))
		{
			return hResult;
		}

		hResult = GetAudioDeviceSourceAttribute(pSourceActivateIndex, pDevsourceAttributeIndex);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	m_pDevsourceAttribute = move(pDevsourceAttribute);
	m_ppSourceActivate = ppSourceActivate;
	m_cSourceActivate = cSourceActivate;
	*pSourceCount = cSourceActivate;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::EnumVideoDeviceSources(UINT32 *pSourceCount)
{
	IMFActivate **ppSourceActivate = nullptr;
	UINT32 cSourceActivate = 0U;

	HRESULT hResult = EnumMediaDeviceSources(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID, &ppSourceActivate, &cSourceActivate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto pDevsourceAttribute = make_unique<struct MF_DEVSOURCE_ATTRIBUTE[]>(cSourceActivate);
	for (UINT32 Index = 0; Index < cSourceActivate; Index++)
	{
		IMFActivate *pSourceActivateIndex = ppSourceActivate[Index];
		auto pDevsourceAttributeIndex = &pDevsourceAttribute[Index];

		hResult = GetMediaDeviceSourceAttribute(pSourceActivateIndex, pDevsourceAttributeIndex);
		if (FAILED(hResult))
		{
			return hResult;
		}

		hResult = GetVideoDeviceSourceAttribute(pSourceActivateIndex, pDevsourceAttributeIndex);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	m_pDevsourceAttribute = move(pDevsourceAttribute);
	m_ppSourceActivate = ppSourceActivate;
	m_cSourceActivate = cSourceActivate;
	*pSourceCount = cSourceActivate;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetFriendlyName(UINT32 SourceIndex, WCHAR **ppFriendlyName)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*ppFriendlyName = m_pDevsourceAttribute[SourceIndex].FriendlyName.get();
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetMediaType(UINT32 SourceIndex, MFT_REGISTER_TYPE_INFO *pMediaType)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*pMediaType = m_pDevsourceAttribute[SourceIndex].MediaType;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetSourceType(UINT32 SourceIndex, GUID *pSourceType)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*pSourceType = m_pDevsourceAttribute[SourceIndex].SourceType;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetSourceTypeAudcapEndpointID(UINT32 SourceIndex, WCHAR **ppSourceTypeAudcapEndpointID)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*ppSourceTypeAudcapEndpointID = m_pDevsourceAttribute[SourceIndex].SourceTypeAudcapEndpointID.get();
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetSourceTypeVidcapCategory(UINT32 SourceIndex, GUID *pSourceTypeVidcapCategory)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*pSourceTypeVidcapCategory = m_pDevsourceAttribute[SourceIndex].SourceTypeVidcapCategory;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetSourceTypeVidcapHWSource(UINT32 SourceIndex, UINT32 *pSourceTypeVidcapHWSource)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*pSourceTypeVidcapHWSource = m_pDevsourceAttribute[SourceIndex].SourceTypeVidcapHWSource;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetSourceTypeVidcapSymbolicLink(UINT32 SourceIndex, WCHAR **ppSourceTypeVidcapSymbolicLink)
{
	if (m_cSourceActivate <= SourceIndex)
	{
		return S_FALSE;
	}

	*ppSourceTypeVidcapSymbolicLink = m_pDevsourceAttribute[SourceIndex].SourceTypeVidcapSymbolicLink.get();
	return S_OK;
}

HRESULT MediaDeviceSourceSet::EnumMediaDeviceSources(GUID MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID, IMFActivate ***pppSourceActivate, UINT32 *pcSourceActivate)
{
	ComPtr<IMFAttributes> IMFAttributesPtr = nullptr;
	HRESULT hResult = MFCreateAttributes(&IMFAttributesPtr, 1U);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFAttributesPtr->SetGUID(
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
		MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_CAPTURE_GUID
	);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = MFEnumDeviceSources(IMFAttributesPtr.Get(), pppSourceActivate, pcSourceActivate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	if (0 == *pcSourceActivate)
	{
		return MF_E_NOT_FOUND;
	}

	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetMediaDeviceSourceAttribute(IMFActivate * pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE * pDevsourceAttributeIndex)
{
	UINT32 cchLength = 0U;
	HRESULT hResult = pSourceActivateIndex->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &cchLength);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cchLength++;
	unique_ptr<WCHAR[]> FriendlyName = make_unique<WCHAR[]>(cchLength);
	hResult = pSourceActivateIndex->GetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, FriendlyName.get(), cchLength, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	MFT_REGISTER_TYPE_INFO MediaType = { 0 };
	hResult = pSourceActivateIndex->GetBlob(MF_DEVSOURCE_ATTRIBUTE_MEDIA_TYPE, reinterpret_cast<UINT8*>(&MediaType), sizeof(MFT_REGISTER_TYPE_INFO), nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID SourceType = { 0 };
	hResult = pSourceActivateIndex->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &SourceType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pDevsourceAttributeIndex->FriendlyName = move(FriendlyName);
	pDevsourceAttributeIndex->MediaType = MediaType;
	pDevsourceAttributeIndex->SourceType = SourceType;
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetVideoDeviceSourceAttribute(IMFActivate *pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE *pDevsourceAttributeIndex)
{
	GUID SourceTypeVidcapCategory = { 0 };
	HRESULT hResult = pSourceActivateIndex->GetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_CATEGORY, &SourceTypeVidcapCategory);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 SourceTypeVidcapHWSource = 0U;
	hResult = pSourceActivateIndex->GetUINT32(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_HW_SOURCE, &SourceTypeVidcapHWSource);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 cchLength = 0U;
	hResult = pSourceActivateIndex->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &cchLength);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cchLength++;
	unique_ptr<WCHAR[]> SourceTypeVidcapSymbolicLink = make_unique<WCHAR[]>(cchLength);
	hResult = pSourceActivateIndex->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, SourceTypeVidcapSymbolicLink.get(), cchLength, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pDevsourceAttributeIndex->SourceTypeAudcapEndpointID = nullptr;
	pDevsourceAttributeIndex->SourceTypeVidcapCategory = SourceTypeVidcapCategory;
	pDevsourceAttributeIndex->SourceTypeVidcapHWSource = SourceTypeVidcapHWSource;
	pDevsourceAttributeIndex->SourceTypeVidcapSymbolicLink = move(SourceTypeVidcapSymbolicLink);
	return S_OK;
}

HRESULT MediaDeviceSourceSet::GetAudioDeviceSourceAttribute(IMFActivate * pSourceActivateIndex, MF_DEVSOURCE_ATTRIBUTE * pDevsourceAttributeIndex)
{
	UINT32 cchLength = 0U;
	HRESULT hResult = pSourceActivateIndex->GetStringLength(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID, &cchLength);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cchLength++;
	unique_ptr<WCHAR[]> SourceTypeAudcapEndpointID = make_unique<WCHAR[]>(cchLength);
	hResult = pSourceActivateIndex->GetString(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_ENDPOINT_ID, SourceTypeAudcapEndpointID.get(), cchLength, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pDevsourceAttributeIndex->SourceTypeAudcapEndpointID = move(SourceTypeAudcapEndpointID);
	pDevsourceAttributeIndex->SourceTypeVidcapCategory = { 0 };
	pDevsourceAttributeIndex->SourceTypeVidcapHWSource = 0U;
	pDevsourceAttributeIndex->SourceTypeVidcapSymbolicLink = nullptr;
	return S_OK;
}

class MediaSource
{
public:
	MediaSource();
	~MediaSource();
	HRESULT Create(ComPtr<IMFMediaSource> IMFMediaSourcePtr);

private:
	typedef struct Stream
	{
		BOOL						fSelected;
		ComPtr<IMFStreamDescriptor> DescriptorPtr;
		ComPtr<IMFMediaTypeHandler> MediaTypeHandlerPtr;
		GUID						guidMajorType;
		DWORD						dwTypeCount;
		DWORD						dwStreamIdentifier;
	}Stream;

	ComPtr<IMFMediaSource>				m_MediaSourcePtr;
	ComPtr<IMFPresentationDescriptor>	m_PresentationDescriptorPtr;
	unique_ptr<Stream[]>				m_pStream;
	DWORD								m_dwDescriptorCount;
	DWORD								m_dwCharacteristics;

	HRESULT GetStream(ComPtr<IMFPresentationDescriptor>	IMFPresentationDescriptorPtr, DWORD dwDescriptorIndex, Stream *pStreamIndex);
};

MediaSource::MediaSource() :
	m_MediaSourcePtr(nullptr),
	m_PresentationDescriptorPtr(nullptr),
	m_pStream(nullptr),
	m_dwDescriptorCount(0UL),
	m_dwCharacteristics(0UL)
{
}

MediaSource::~MediaSource()
{
}

HRESULT MediaSource::Create(ComPtr<IMFMediaSource> MediaSourcePtr)
{
	ComPtr<IMFPresentationDescriptor> PresentationDescriptorPtr = nullptr;
	HRESULT hResult = MediaSourcePtr->CreatePresentationDescriptor(&PresentationDescriptorPtr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD dwDescriptorCount = 0UL;
	hResult = PresentationDescriptorPtr->GetStreamDescriptorCount(&dwDescriptorCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto pStream = make_unique<Stream[]>(dwDescriptorCount);
	for (DWORD dwIndex = 0; dwIndex < dwDescriptorCount; dwIndex++)
	{
		auto pStreamIndex = &pStream[dwIndex];
		hResult = GetStream(PresentationDescriptorPtr, dwIndex, pStreamIndex);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	DWORD dwCharacteristics = 0UL;
	hResult = MediaSourcePtr->GetCharacteristics(&dwCharacteristics);
	if (FAILED(hResult))
	{
		return hResult;
	}

	m_MediaSourcePtr = MediaSourcePtr;
	m_PresentationDescriptorPtr = PresentationDescriptorPtr;
	m_pStream = move(pStream);
	m_dwDescriptorCount = dwDescriptorCount;
	m_dwCharacteristics = dwCharacteristics;
	return S_OK;
}

HRESULT MediaSource::GetStream(ComPtr<IMFPresentationDescriptor> IMFPresentationDescriptorPtr, DWORD dwDescriptorIndex, Stream *pStreamIndex)
{
	BOOL fSelected = FALSE;
	ComPtr<IMFStreamDescriptor> DescriptorPtr = nullptr;

	HRESULT hResult = IMFPresentationDescriptorPtr->GetStreamDescriptorByIndex(dwDescriptorIndex, &fSelected, &DescriptorPtr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	ComPtr<IMFMediaTypeHandler> MediaTypeHandlerPtr = nullptr;
	hResult = DescriptorPtr->GetMediaTypeHandler(&MediaTypeHandlerPtr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID guidMajorType = { 0 };
	hResult = MediaTypeHandlerPtr->GetMajorType(&guidMajorType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD dwTypeCount = 0UL;
	hResult = MediaTypeHandlerPtr->GetMediaTypeCount(&dwTypeCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD dwStreamIdentifier = 0UL;
	hResult = DescriptorPtr->GetStreamIdentifier(&dwStreamIdentifier);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pStreamIndex->fSelected = fSelected;
	pStreamIndex->DescriptorPtr = DescriptorPtr;
	pStreamIndex->MediaTypeHandlerPtr = MediaTypeHandlerPtr;
	pStreamIndex->guidMajorType = guidMajorType;
	pStreamIndex->dwTypeCount = dwTypeCount;
	pStreamIndex->dwStreamIdentifier = dwStreamIdentifier;
	return S_OK;
}

class MediaType
{
public:
	MediaType();
	~MediaType();
	HRESULT Create(ComPtr<IMFMediaType> MediaTypePtr, bool FullAttributes);

private:
	typedef struct GENERAL_FORMAT_ATTRIBUTES
	{
		GUID				MajorType;
		GUID				SubType;
		UINT32				AllSamplesIndependent;
		UINT32				FixedSizeSamples;
		UINT32				Compressed;
		UINT32				SampleSize;
		unique_ptr<UINT8[]>	pWrappedType;
		unique_ptr<UINT8[]>	pUserData;
		GUID				AmFormatType;
	}GENERAL_FORMAT_ATTRIBUTES;

	typedef struct AUDIO_FORMAT_ATTRIBUTES
	{
		UINT32				AudioNumChannels;
		UINT32				AudioSamplesPerSecond;
		double				AudioFloatSamplesPerSecond;
		UINT32				AudioAVGBytesPerSecond;
		UINT32				AudioBlockAlignment;
		UINT32				AudioBitsPerSample;
		UINT32				AudioValidBitsPerSample;
		UINT32				AudioSamplesPerBlock;
		UINT32				AudioChannelMask;
		unique_ptr<UINT8[]> AudioFolddownMatrix;
		UINT32				AudioWMADRCPeakRef;
		UINT32				AudioWMADRCPeakTarget;
		UINT32				AudioWMADRCAVGRef;
		UINT32				AudioWMADRCAVGTarget;
		UINT32				AudioPreferWAVEFormatEX;
		UINT32				AACPayloadType;
		UINT32				AACAudioProfileLevelIndication;
		UINT32				OriginalWAVEFormatTag;
	}AUDIO_FORMAT_ATTRIBUTES;

	typedef struct VIDEO_FORMAT_ATTRIBUTES
	{
		UINT32				Video3D;
		UINT32				VideoRotation;
		UINT64				FrameSize;
		UINT64				FrameRate;
		UINT64				PixelAspectRatio;
		UINT32				DRMFlags;
		UINT32				PadControlFlags;
		UINT32				SourceContentHint;
		UINT32				VideoChromaSiting;
		UINT32				InterlaceMode;
		UINT32				TransferFunction;
		UINT32				VideoPrimaries;
		unique_ptr<UINT8[]>	CustomVideoPrimaries;
		UINT32				YUVMatrix;
		UINT32				VideoLighting;
		UINT32				VideoNominalRange;
		unique_ptr<UINT8[]>	GeometricAperture;
		unique_ptr<UINT8[]>	MinimumDisplayAperture;
		unique_ptr<UINT8[]>	PanScanAperture;
		UINT32				PanScanEnabled;
		UINT32				AVGBitRate;
		UINT32				AVGBitErrorRate;
		UINT32				MAXKeyFrameSpacing;
		UINT32				DefaultStride;
		unique_ptr<UINT8[]>	Palette;
		UINT32				MPEGStartTimeCode;
		UINT32				MPEG2Profile;
		UINT32				MPEG2Level;
		UINT32				MPEG2Flags;
		unique_ptr<UINT8[]>	MPEGSequenceHeader;
		UINT32				CallerAllocatesOutput;
		UINT32				DisableFRC;
		UINT32				Original4CC;
		UINT64				FrameRateRangeMIN;
		UINT64				FrameRateRangeMAX;
	}VIDEO_FORMAT_ATTRIBUTES;

	typedef struct UNCOMPRESSED_AUDIO_TYPES
	{
		GUID	Getmajor_type;
		GUID	Getsubtype;
		UINT32	Getaudio_num_channels;
		UINT32	Getaudio_samples_per_second;
		UINT32	Getaudio_block_alignment;
		UINT32	Getaudio_avg_bytes_per_second;
		UINT32	Getaudio_bits_per_sample;
		UINT32	Getall_samples_independent;

		UINT32	Getaudio_valid_bits_per_sample;
		UINT32	Getaudio_channel_mask;
	}UNCOMPRESSED_AUDIO_TYPES;

	typedef struct UNCOMPRESSED_VIDEO_TYPES
	{
		GUID	Getmajor_type;
		GUID	Getsubtype;
		UINT32	Getdefault_stride;
		UINT64	Getframe_rate;
		UINT64	Getframe_size;
		UINT32	Getinterlace_mode;
		UINT32	Getall_samples_independent;
		UINT64	Getpixel_aspect_ratio;

		UINT32	Getvideo_primaries;
		UINT32	Gettransfer_function;
		UINT32	Getyuv_matrix;
		UINT32	Getvideo_chroma_siting;
		UINT32	Getvideo_nominal_range;
	}UNCOMPRESSED_VIDEO_TYPES;

	ComPtr<IMFMediaType>					m_MediaTypePtr;
	unique_ptr<GENERAL_FORMAT_ATTRIBUTES>	m_pGeneralFormatAttributes;
	unique_ptr<AUDIO_FORMAT_ATTRIBUTES>		m_pAudioFormatAttributes;
	unique_ptr<VIDEO_FORMAT_ATTRIBUTES>		m_pVideoFormatAttributes;

	HRESULT GetGeneralFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, GENERAL_FORMAT_ATTRIBUTES *pGeneralFormatAttributes);
	HRESULT GetAudioFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, AUDIO_FORMAT_ATTRIBUTES *pAudioFormatAttributes);
	HRESULT GetVideoFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, VIDEO_FORMAT_ATTRIBUTES *pVideoFormatAttributes);
};

MediaType::MediaType()
{
}

MediaType::~MediaType()
{
}

HRESULT MediaType::Create(ComPtr<IMFMediaType> MediaTypePtr, bool FullAttributes)
{
	return S_OK;
}

HRESULT MediaType::GetGeneralFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, GENERAL_FORMAT_ATTRIBUTES *pGeneralFormatAttributes)
{
	GUID MajorType = { 0 };
	HRESULT hResult = MediaTypePtr->GetGUID(MF_MT_MAJOR_TYPE, &MajorType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID SubType = { 0 };
	hResult = MediaTypePtr->GetGUID(MF_MT_SUBTYPE, &SubType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AllSamplesIndependent = FALSE;
	hResult = MediaTypePtr->GetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, &AllSamplesIndependent);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 FixedSizeSamples = FALSE;
	hResult = MediaTypePtr->GetUINT32(MF_MT_FIXED_SIZE_SAMPLES, &FixedSizeSamples);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 Compressed = FALSE;
	hResult = MediaTypePtr->GetUINT32(MF_MT_COMPRESSED, &Compressed);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 SampleSize = 0U;
	if (TRUE == FixedSizeSamples)
	{
		hResult = MediaTypePtr->GetUINT32(MF_MT_SAMPLE_SIZE, &SampleSize);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	UINT32 cbWrappedTypeBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_WRAPPED_TYPE, &cbWrappedTypeBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto pWrappedType = make_unique<UINT8[]>(cbWrappedTypeBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_WRAPPED_TYPE, pWrappedType.get(), cbWrappedTypeBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 cbUserDataBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_USER_DATA, &cbUserDataBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto pUserData = make_unique<UINT8[]>(cbUserDataBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_USER_DATA, pUserData.get(), cbUserDataBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID AmFormatType = { 0 };
	hResult = MediaTypePtr->GetGUID(MF_MT_AM_FORMAT_TYPE, &AmFormatType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pGeneralFormatAttributes->MajorType = MajorType;
	pGeneralFormatAttributes->SubType = SubType;
	pGeneralFormatAttributes->AllSamplesIndependent = AllSamplesIndependent;
	pGeneralFormatAttributes->FixedSizeSamples = FixedSizeSamples;
	pGeneralFormatAttributes->Compressed = Compressed;
	pGeneralFormatAttributes->SampleSize = SampleSize;
	pGeneralFormatAttributes->pWrappedType = move(pWrappedType);
	pGeneralFormatAttributes->pUserData = move(pUserData);
	pGeneralFormatAttributes->AmFormatType = AmFormatType;
	return S_OK;
}

HRESULT MediaType::GetAudioFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, AUDIO_FORMAT_ATTRIBUTES *pAudioFormatAttributes)
{
	UINT32 AudioNumChannels = 0U;
	HRESULT hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &AudioNumChannels);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioSamplesPerSecond = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &AudioSamplesPerSecond);
	if (FAILED(hResult))
	{
		return hResult;
	}

	double AudioFloatSamplesPerSecond = 0.0;
	hResult = MediaTypePtr->GetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, &AudioFloatSamplesPerSecond);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioAVGBytesPerSecond = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, &AudioAVGBytesPerSecond);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioBlockAlignment = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &AudioBlockAlignment);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioBitsPerSample = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &AudioBitsPerSample);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioValidBitsPerSample = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, &AudioValidBitsPerSample);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioSamplesPerBlock = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, &AudioSamplesPerBlock);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioChannelMask = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_CHANNEL_MASK, &AudioChannelMask);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_AUDIO_FOLDDOWN_MATRIX, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto AudioFolddownMatrix = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_AUDIO_FOLDDOWN_MATRIX, AudioFolddownMatrix.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioWMADRCPeakRef = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_WMADRC_PEAKREF, &AudioWMADRCPeakRef);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioWMADRCPeakTarget = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_WMADRC_PEAKTARGET, &AudioWMADRCPeakTarget);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioWMADRCAVGRef = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_WMADRC_AVGREF, &AudioWMADRCAVGRef);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioWMADRCAVGTarget = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_WMADRC_AVGTARGET, &AudioWMADRCAVGTarget);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AudioPreferWAVEFormatEX = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AUDIO_PREFER_WAVEFORMATEX, &AudioPreferWAVEFormatEX);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AACPayloadType = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AAC_PAYLOAD_TYPE, &AACPayloadType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AACAudioProfileLevelIndication = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AAC_AUDIO_PROFILE_LEVEL_INDICATION, &AACAudioProfileLevelIndication);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 OriginalWAVEFormatTag = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_ORIGINAL_WAVE_FORMAT_TAG, &OriginalWAVEFormatTag);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pAudioFormatAttributes->AudioNumChannels = AudioNumChannels;
	pAudioFormatAttributes->AudioSamplesPerSecond = AudioSamplesPerSecond;
	pAudioFormatAttributes->AudioFloatSamplesPerSecond = AudioFloatSamplesPerSecond;
	pAudioFormatAttributes->AudioAVGBytesPerSecond = AudioAVGBytesPerSecond;
	pAudioFormatAttributes->AudioBlockAlignment = AudioBlockAlignment;
	pAudioFormatAttributes->AudioBitsPerSample = AudioBitsPerSample;
	pAudioFormatAttributes->AudioValidBitsPerSample = AudioValidBitsPerSample;
	pAudioFormatAttributes->AudioSamplesPerBlock = AudioSamplesPerBlock;
	pAudioFormatAttributes->AudioChannelMask = AudioChannelMask;
	pAudioFormatAttributes->AudioFolddownMatrix = move(AudioFolddownMatrix);
	pAudioFormatAttributes->AudioWMADRCPeakRef = AudioWMADRCPeakRef;
	pAudioFormatAttributes->AudioWMADRCPeakTarget = AudioWMADRCPeakTarget;
	pAudioFormatAttributes->AudioWMADRCAVGRef = AudioWMADRCAVGRef;
	pAudioFormatAttributes->AudioWMADRCAVGTarget = AudioWMADRCAVGTarget;
	pAudioFormatAttributes->AudioPreferWAVEFormatEX = AudioPreferWAVEFormatEX;
	pAudioFormatAttributes->AACPayloadType = AACPayloadType;
	pAudioFormatAttributes->AACAudioProfileLevelIndication = AACAudioProfileLevelIndication;
	pAudioFormatAttributes->OriginalWAVEFormatTag = OriginalWAVEFormatTag;
	return S_OK;
}

HRESULT MediaType::GetVideoFormatAttributes(ComPtr<IMFMediaType> MediaTypePtr, VIDEO_FORMAT_ATTRIBUTES *pVideoFormatAttributes)
{
	UINT32 Video3D = 0U;
	HRESULT hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_3D, &Video3D);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 VideoRotation = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_ROTATION, &VideoRotation);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 FrameSize = 0ULL;
	hResult = MediaTypePtr->GetUINT64(MF_MT_FRAME_SIZE, &FrameSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 FrameRate = 0ULL;
	hResult = MediaTypePtr->GetUINT64(MF_MT_FRAME_RATE, &FrameRate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 PixelAspectRatio = 0ULL;
	hResult = MediaTypePtr->GetUINT64(MF_MT_PIXEL_ASPECT_RATIO, &PixelAspectRatio);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 DRMFlags = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_DRM_FLAGS, &DRMFlags);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 PadControlFlags = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_PAD_CONTROL_FLAGS, &PadControlFlags);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 SourceContentHint = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_SOURCE_CONTENT_HINT, &SourceContentHint);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 VideoChromaSiting = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_CHROMA_SITING, &VideoChromaSiting);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 InterlaceMode = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_INTERLACE_MODE, &InterlaceMode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 TransferFunction = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_TRANSFER_FUNCTION, &TransferFunction);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 VideoPrimaries = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_PRIMARIES, &VideoPrimaries);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_CUSTOM_VIDEO_PRIMARIES, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto CustomVideoPrimaries = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, CustomVideoPrimaries.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 YUVMatrix = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_YUV_MATRIX, &YUVMatrix);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 VideoLighting = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_LIGHTING, &VideoLighting);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 VideoNominalRange = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, &VideoNominalRange);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_GEOMETRIC_APERTURE, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto GeometricAperture = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_GEOMETRIC_APERTURE, GeometricAperture.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_MINIMUM_DISPLAY_APERTURE, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto MinimumDisplayAperture = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, MinimumDisplayAperture.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_PAN_SCAN_APERTURE, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto PanScanAperture = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_PAN_SCAN_APERTURE, PanScanAperture.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 PanScanEnabled = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_PAN_SCAN_ENABLED, &PanScanEnabled);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AVGBitRate = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AVG_BITRATE, &AVGBitRate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 AVGBitErrorRate = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_AVG_BIT_ERROR_RATE, &AVGBitErrorRate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 MAXKeyFrameSpacing = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_MAX_KEYFRAME_SPACING, &MAXKeyFrameSpacing);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 DefaultStride = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_DEFAULT_STRIDE, &DefaultStride);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_PALETTE, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto Palette = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_PALETTE, Palette.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 MPEGStartTimeCode = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_MPEG_START_TIME_CODE, &MPEGStartTimeCode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 MPEG2Profile = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_MPEG2_PROFILE, &MPEG2Profile);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 MPEG2Level = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_MPEG2_LEVEL, &MPEG2Level);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 MPEG2Flags = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_MPEG2_FLAGS, &MPEG2Flags);
	if (FAILED(hResult))
	{
		return hResult;
	}

	cbBlobSize = 0U;
	hResult = MediaTypePtr->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, &cbBlobSize);
	if (FAILED(hResult))
	{
		return hResult;
	}

	auto MPEGSequenceHeader = make_unique<UINT8[]>(cbBlobSize);
	hResult = MediaTypePtr->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, MPEGSequenceHeader.get(), cbBlobSize, nullptr);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 CallerAllocatesOutput = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_XVP_CALLER_ALLOCATES_OUTPUT, &CallerAllocatesOutput);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 DisableFRC = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_XVP_DISABLE_FRC, &DisableFRC);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT32 Original4CC = 0U;
	hResult = MediaTypePtr->GetUINT32(MF_MT_ORIGINAL_4CC, &Original4CC);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 FrameRateRangeMIN = 0ULL;
	hResult = MediaTypePtr->GetUINT64(MF_MT_FRAME_RATE_RANGE_MIN, &FrameRateRangeMIN);
	if (FAILED(hResult))
	{
		return hResult;
	}

	UINT64 FrameRateRangeMAX = 0ULL;
	hResult = MediaTypePtr->GetUINT64(MF_MT_FRAME_RATE_RANGE_MAX, &FrameRateRangeMAX);
	if (FAILED(hResult))
	{
		return hResult;
	}

	pVideoFormatAttributes->Video3D = Video3D;
	pVideoFormatAttributes->VideoRotation = VideoRotation;
	pVideoFormatAttributes->FrameSize = FrameSize;
	pVideoFormatAttributes->FrameRate = FrameRate;
	pVideoFormatAttributes->PixelAspectRatio = PixelAspectRatio;
	pVideoFormatAttributes->DRMFlags = DRMFlags;
	pVideoFormatAttributes->PadControlFlags = PadControlFlags;
	pVideoFormatAttributes->SourceContentHint = SourceContentHint;
	pVideoFormatAttributes->VideoChromaSiting = VideoChromaSiting;
	pVideoFormatAttributes->InterlaceMode = InterlaceMode;
	pVideoFormatAttributes->TransferFunction = TransferFunction;
	pVideoFormatAttributes->VideoPrimaries = VideoPrimaries;
	pVideoFormatAttributes->CustomVideoPrimaries = move(CustomVideoPrimaries);
	pVideoFormatAttributes->YUVMatrix = YUVMatrix;
	pVideoFormatAttributes->VideoLighting = VideoLighting;
	pVideoFormatAttributes->VideoNominalRange = VideoNominalRange;
	pVideoFormatAttributes->GeometricAperture = move(GeometricAperture);
	pVideoFormatAttributes->MinimumDisplayAperture = move(MinimumDisplayAperture);
	pVideoFormatAttributes->PanScanAperture = move(PanScanAperture);
	pVideoFormatAttributes->PanScanEnabled = PanScanEnabled;
	pVideoFormatAttributes->AVGBitRate = AVGBitRate;
	pVideoFormatAttributes->AVGBitErrorRate = AVGBitErrorRate;
	pVideoFormatAttributes->MAXKeyFrameSpacing = MAXKeyFrameSpacing;
	pVideoFormatAttributes->DefaultStride = DefaultStride;
	pVideoFormatAttributes->Palette = move(Palette);
	pVideoFormatAttributes->MPEGStartTimeCode = MPEGStartTimeCode;
	pVideoFormatAttributes->MPEG2Profile = MPEG2Profile;
	pVideoFormatAttributes->MPEG2Level = MPEG2Level;
	pVideoFormatAttributes->MPEG2Flags = MPEG2Flags;
	pVideoFormatAttributes->MPEGSequenceHeader = move(MPEGSequenceHeader);
	pVideoFormatAttributes->CallerAllocatesOutput = CallerAllocatesOutput;
	pVideoFormatAttributes->DisableFRC = DisableFRC;
	pVideoFormatAttributes->Original4CC = Original4CC;
	pVideoFormatAttributes->FrameRateRangeMIN = FrameRateRangeMIN;
	pVideoFormatAttributes->FrameRateRangeMAX = FrameRateRangeMAX;
	return S_OK;
}
