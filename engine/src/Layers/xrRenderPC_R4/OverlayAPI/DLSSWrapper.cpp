#include "stdafx.h"

#include "DLSSWrapper.h"
#include "DLSSParameters.h"

DLSSWrapper g_DLSSWrapper;

bool DLSSInited = false;
extern ENGINE_API u32 ps_render_scale_preset;
extern ENGINE_API float ps_render_scale;

void DLSSWrapper::Create(const ContextParameters& Parameters)
{
	PROF_EVENT("DLSSWrapper::Create");
	if (m_created)
	{
		Destroy();
	}
	
	if (RFeatureLevel < D3D_FEATURE_LEVEL_11_1)
	{
		return;
	}

#ifdef IXR_X64
	NVSDK_NGX_Result result;
	if (!DLSSInited)
	{
		result = NVSDK_NGX_D3D11_Init(1602, L"", RDevice);
		if (result != NVSDK_NGX_Result_Success) {
			return;
		}

		DLSSInited = true;
	}

	result = NVSDK_NGX_D3D11_GetCapabilityParameters(&NgxParameters);
	if (result != NVSDK_NGX_Result_Success)
	{
		return;
	}

	uint32_t needsUpdatedDriver = 1;
	result = NgxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver, &needsUpdatedDriver);
	if (needsUpdatedDriver)
	{
		Msg("! PLEASE UPDATE YOUR DRIVER");
	}

	uint32_t dlssAvailable = 0;
	result = NgxParameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &dlssAvailable);
	if (!dlssAvailable)
	{
		NVSDK_NGX_D3D11_DestroyParameters(NgxParameters);
		NgxParameters = nullptr;
		return;
	}
    // COP_ENGINE_PATCH: COP-ENG-174 - do not leave NGX in default Performance
    // while setting only the Quality hint; null exposure requires AutoExposure.
    auto dlssCreateParams = dlss_contract::creation(ps_render_scale_preset, ps_render_scale,
        Parameters.renderSize.x, Parameters.renderSize.y, Parameters.displaySize.x, Parameters.displaySize.y);
    NgxParameters->Set(dlss_contract::hint(dlssCreateParams.Feature.InPerfQualityValue),
        static_cast<int>(NVSDK_NGX_DLSS_Hint_Render_Preset_F));
    m_loggedEvaluations = 0;
	result = NGX_D3D11_CREATE_DLSS_EXT(RContext, &Handle, NgxParameters, &dlssCreateParams);
    Msg("* [DLSS contract174] create result=%08x render=%ux%u target=%ux%u quality=%d flags=%x",
        unsigned(result), dlssCreateParams.Feature.InWidth, dlssCreateParams.Feature.InHeight,
        dlssCreateParams.Feature.InTargetWidth, dlssCreateParams.Feature.InTargetHeight,
        int(dlssCreateParams.Feature.InPerfQualityValue), unsigned(dlssCreateParams.InFeatureCreateFlags));
    if (result != NVSDK_NGX_Result_Success || Handle == nullptr) return; // COP_ENGINE_PATCH: COP-ENG-177
    DisplaySize = Parameters.displaySize;
    m_created = true;
#endif // IXR_X64
}

void DLSSWrapper::Destroy() 
{
#ifdef IXR_X64
	//if (!m_created) {
	//    return;
	//}

	if (Handle != nullptr)
	{
		NVSDK_NGX_D3D11_ReleaseFeature(Handle);
		Handle = nullptr;
	}

	if (NgxParameters != nullptr)
	{
		NVSDK_NGX_D3D11_DestroyParameters(NgxParameters);
		NgxParameters = nullptr;
	}

	if (DLSSInited)
	{
		NVSDK_NGX_D3D11_Shutdown1(nullptr);
		DLSSInited = false;
	}

	m_created = false;
#endif // IXR_X64
}

bool DLSSWrapper::Draw(const DrawParameters& params)
{
	if(!m_created)
	{
		Msg("! DLSSWrapper not created. Need use FSR");
		return false;
	}

#ifdef IXR_X64
	ID3D11Resource* resourceInput = params.unresolvedColorResource;
	ID3D11Resource* resourceMv = params.motionvectorResource;
	ID3D11Resource* resourceDepth = params.depthbufferResource;
	ID3D11Resource* resourceOutput = params.resolvedColorResource;

	NVSDK_NGX_D3D11_DLSS_Eval_Params dlssEvalParams = {};

	dlssEvalParams.Feature.pInColor = resourceInput;
	dlssEvalParams.Feature.pInOutput = resourceOutput;
	dlssEvalParams.Feature.InSharpness = params.sharpness;

	dlssEvalParams.pInDepth = resourceDepth;
	dlssEvalParams.pInMotionVectors = resourceMv;

	dlssEvalParams.InRenderSubrectDimensions.Width = params.renderWidth;
	dlssEvalParams.InRenderSubrectDimensions.Height = params.renderHeight;

	dlssEvalParams.InJitterOffsetX = params.cameraJitterX;
	dlssEvalParams.InJitterOffsetY = params.cameraJitterY;

	dlssEvalParams.InReset = params.cameraReset;
    // COP_ENGINE_PATCH: COP-ENG-174 - this input is scene-linear, not pre-exposed.
    dlssEvalParams.pInExposureTexture = nullptr; // AutoExposure owns adaptation inside NGX.
    dlssEvalParams.InPreExposure = 1.f;
    dlssEvalParams.InExposureScale = 1.f;
    dlssEvalParams.InFrameTimeDeltaInMsec = dlss_contract::frame_time(params.frameTimeDelta);


	dlssEvalParams.InMVScaleX = -(float)params.renderWidth * 0.5f;    // adjust the x direction in motion vector to fit FSR2's requirement
	dlssEvalParams.InMVScaleY = (float)params.renderHeight * 0.5f;

	dlssEvalParams.pInTransparencyMask = params.transparencyAndCompositionResource;
	
	NVSDK_NGX_Result result = NGX_D3D11_EVALUATE_DLSS_EXT(RContext, Handle, NgxParameters, &dlssEvalParams);
    if (m_loggedEvaluations < 4 || (params.cameraReset && m_loggedEvaluations < 32))
    {
        Msg("* [DLSS contract174] eval result=%08x reset=%d render=%dx%d jitter=(%.6g,%.6g) dt=%.6g exposure=auto",
            unsigned(result), int(params.cameraReset), params.renderWidth, params.renderHeight,
            params.cameraJitterX, params.cameraJitterY, dlssEvalParams.InFrameTimeDeltaInMsec);
        ++m_loggedEvaluations;
    }
	if(result != NVSDK_NGX_Result_Success)
	{
		Msg("! NGX_D3D11_EVALUATE_DLSS_EXT not valid. Need use FSR");
		return false;
	}
#endif // IXR_X64

	return true;
}

DLSSWrapper::~DLSSWrapper()
{
	Destroy();
}
