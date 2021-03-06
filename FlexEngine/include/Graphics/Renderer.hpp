#pragma once

IGNORE_WARNINGS_PUSH
#include "LinearMath/btIDebugDraw.h"
IGNORE_WARNINGS_POP

#include "Physics/PhysicsDebuggingSettings.hpp"
#include "RendererTypes.hpp"
#include "VertexBufferData.hpp"

class btIDebugDraw;
struct FT_LibraryRec_;
struct FT_FaceRec_;
typedef struct FT_LibraryRec_* FT_Library;
typedef struct FT_FaceRec_* FT_Face;

namespace flex
{
	class BitmapFont;
	class DirectionalLight;
	class DirectoryWatcher;
	class GameObject;
	class MeshComponent;
	class Mesh;
	class ParticleSystem;
	class PointLight;
	struct TextCache;
	struct FontMetaData;
	struct FontMetric;

	class PhysicsDebugDrawBase : public btIDebugDraw
	{
	public:
		virtual void Initialize() = 0;
		virtual void Destroy() = 0;
		virtual void DrawLineWithAlpha(const btVector3& from, const btVector3& to, const btVector4& color) = 0;
		virtual void DrawLineWithAlpha(const btVector3& from, const btVector3& to, const btVector4& colorFrom, const btVector4& colorTo) = 0;

		virtual void OnPostSceneChange() = 0;

		void UpdateDebugMode();
		void ClearLines();

		virtual void flushLines() override;

	protected:
		virtual void Draw() = 0;

		struct LineSegment
		{
			LineSegment() {}

			LineSegment(const btVector3& vStart, const btVector3& vEnd, const btVector3& vColFrom, const btVector3& vColTo)
			{
				memcpy(start, vStart.m_floats, sizeof(real) * 3);
				memcpy(end, vEnd.m_floats, sizeof(real) * 3);
				memcpy(colorFrom, vColFrom.m_floats, sizeof(real) * 3);
				memcpy(colorTo, vColTo.m_floats, sizeof(real) * 3);
				colorFrom[3] = 1.0f;
				colorTo[3] = 1.0f;
			}
			LineSegment(const btVector3& vStart, const btVector3& vEnd, const btVector4& vColFrom, const btVector3& vColTo)
			{
				memcpy(start, vStart.m_floats, sizeof(real) * 3);
				memcpy(end, vEnd.m_floats, sizeof(real) * 3);
				memcpy(colorFrom, vColFrom.m_floats, sizeof(real) * 4);
				memcpy(colorTo, vColTo.m_floats, sizeof(real) * 4);
			}
			real start[3];
			real end[3];
			real colorFrom[4];
			real colorTo[4];
		};

		static const u32 MAX_NUM_LINE_SEGMENTS = 65536;
		u32 m_LineSegmentIndex = 0;
		LineSegment m_LineSegments[MAX_NUM_LINE_SEGMENTS];

		i32 m_DebugMode = 0;

	};

	class Renderer
	{
	public:
		Renderer();
		virtual ~Renderer();

		virtual void Initialize();
		virtual void PostInitialize();
		virtual void Destroy();

		virtual MaterialID InitializeMaterial(const MaterialCreateInfo* createInfo, MaterialID matToReplace = InvalidMaterialID) = 0;
		virtual TextureID InitializeTextureFromFile(const std::string& relativeFilePath, i32 channelCount, bool bFlipVertically, bool bGenerateMipMaps, bool bHDR) = 0;
		virtual TextureID InitializeTextureFromMemory(void* data, u32 size, VkFormat inFormat, const std::string& name, u32 width, u32 height, u32 channelCount, VkFilter inFilter) = 0;
		virtual RenderID InitializeRenderObject(const RenderObjectCreateInfo* createInfo) = 0;
		virtual void PostInitializeRenderObject(RenderID renderID) = 0; // Only call when creating objects after calling PostInitialize()
		virtual void DestroyTexture(TextureID textureID) = 0;

		virtual void ClearMaterials(bool bDestroyPersistentMats = false) = 0;

		virtual void SetTopologyMode(RenderID renderID, TopologyMode topology) = 0;
		virtual void SetClearColor(real r, real g, real b) = 0;

		virtual void Update();
		virtual void Draw() = 0;
		virtual void DrawImGuiMisc();
		virtual void DrawImGuiWindows();

		virtual void UpdateDynamicVertexData(RenderID renderID, VertexBufferData const* vertexBufferData, const std::vector<u32>& indexData) = 0;
		virtual void FreeDynamicVertexData(RenderID renderID) = 0;
		virtual void ShrinkDynamicVertexData(RenderID renderID, real minUnused = 0.0f) = 0;
		virtual u32 GetDynamicVertexBufferSize(RenderID renderID) = 0;
		virtual u32 GetDynamicVertexBufferUsedSize(RenderID renderID) = 0;

		void DrawImGuiForGameObject(GameObject* gameObject);

		virtual void RecompileShaders(bool bForce) = 0;
		virtual void LoadFonts(bool bForceRender) = 0;

		virtual void ReloadSkybox(bool bRandomizeTexture) = 0;

		virtual void OnWindowSizeChanged(i32 width, i32 height) = 0;

		virtual void OnPreSceneChange() = 0;
		virtual void OnPostSceneChange(); // Called once scene has been loaded and all objects have been initialized and post initialized

		/*
		* Fills outInfo with an up-to-date version of the render object's info
		* Returns true if renderID refers to a valid render object
		*/
		virtual bool GetRenderObjectCreateInfo(RenderID renderID, RenderObjectCreateInfo& outInfo) = 0;

		virtual void SetVSyncEnabled(bool bEnableVSync) = 0;

		virtual u32 GetRenderObjectCount() const = 0;
		virtual u32 GetRenderObjectCapacity() const = 0;

		virtual void DescribeShaderVariable(RenderID renderID, const std::string& variableName, i32 size, DataType dataType, bool normalized,
			i32 stride, void* pointer) = 0;

		virtual void SetSkyboxMesh(Mesh* skyboxMesh) = 0;

		virtual void SetRenderObjectMaterialID(RenderID renderID, MaterialID materialID) = 0;

		virtual Material& GetMaterial(MaterialID matID) = 0;
		virtual Shader& GetShader(ShaderID shaderID) = 0;

		virtual bool FindOrCreateMaterialByName(const std::string& materialName, MaterialID& materialID) = 0;
		virtual MaterialID GetRenderObjectMaterialID(RenderID renderID) = 0;
		virtual bool GetShaderID(const std::string& shaderName, ShaderID& shaderID) = 0;

		virtual std::vector<Pair<std::string, MaterialID>> GetValidMaterialNames() const = 0;

		virtual bool DestroyRenderObject(RenderID renderID) = 0;

		virtual void SetGlobalUniform(u64 uniform, void* data, u32 dataSize) = 0;

		virtual void NewFrame() = 0;

		virtual void SetReflectionProbeMaterial(MaterialID reflectionProbeMaterialID);

		virtual PhysicsDebugDrawBase* GetDebugDrawer() = 0;

		virtual void DrawStringSS(const std::string& str,
			const glm::vec4& color,
			AnchorPoint anchor,
			const glm::vec2& pos,
			real spacing,
			real scale = 1.0f) = 0;

		virtual void DrawStringWS(const std::string& str,
			const glm::vec4& color,
			const glm::vec3& pos,
			const glm::quat& rot,
			real spacing,
			real scale = 1.0f) = 0;

		virtual void DrawAssetWindowsImGui(bool* bMaterialWindowShowing, bool* bShaderWindowShowing, bool* bTextureWindowShowing, bool* bMeshWindowShowing) = 0;
		virtual void DrawImGuiForRenderObject(RenderID renderID) = 0;

		virtual void RecaptureReflectionProbe() = 0;

		// Call whenever a user-controlled field, such as visibility, changes to rebatch render objects
		virtual void RenderObjectStateChanged() = 0;

		virtual ParticleSystemID AddParticleSystem(const std::string& name, ParticleSystem* system, i32 particleCount) = 0;
		virtual bool RemoveParticleSystem(ParticleSystemID particleSystemID) = 0;

		virtual void RecreateEverything() = 0;

		void DrawImGuiRenderObjects();
		void DrawImGuiSettings();

		real GetStringWidth(const std::string& str, BitmapFont* font, real letterSpacing, bool bNormalized) const;
		real GetStringHeight(const std::string& str, BitmapFont* font, bool bNormalized) const;

		real GetStringWidth(const TextCache& textCache, BitmapFont* font) const;
		real GetStringHeight(const TextCache& textCache, BitmapFont* font) const;

		void SaveSettingsToDisk(bool bAddEditorStr = true);
		void LoadSettingsFromDisk();

		// Pos should lie in range [-1, 1], with y increasing upward
		// Output pos lies in range [0, 1], with y increasing downward,
		// Output scale lies in range [0, 1] - both outputs corrected for aspect ratio
		void TransformRectToScreenSpace(const glm::vec2& pos,
			const glm::vec2& scale,
			glm::vec2& posOut,
			glm::vec2& scaleOut);

		void NormalizeSpritePos(const glm::vec2& pos,
			AnchorPoint anchor,
			const glm::vec2& scale,
			glm::vec2& posOut,
			glm::vec2& scaleOut);

		void EnqueueUntexturedQuad(const glm::vec2& pos, AnchorPoint anchor, const glm::vec2& size, const glm::vec4& color);
		void EnqueueUntexturedQuadRaw(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
		void EnqueueSprite(const SpriteQuadDrawInfo& drawInfo);

		void ToggleRenderGrid();
		bool IsRenderingGrid() const;
		void SetRenderGrid(bool bRenderGrid);

		void SetDisplayBoundingVolumesEnabled(bool bEnabled);
		bool IsDisplayBoundingVolumesEnabled()const;

		PhysicsDebuggingSettings& GetPhysicsDebuggingSettings();

		bool RegisterDirectionalLight(DirectionalLight* dirLight);
		PointLightID RegisterPointLight(PointLightData* pointLightData);
		void UpdatePointLightData(PointLightID ID, PointLightData* data);

		void RemoveDirectionalLight();
		void RemovePointLight(PointLightID ID);
		void RemoveAllPointLights();

		DirLightData* GetDirectionalLight();
		PointLightData* GetPointLight(PointLightID ID);
		i32 GetNumPointLights();

		i32 GetFramesRenderedCount() const;

		BitmapFont* SetFont(std::string fontID);
		// Draws the given string in the center of the screen for a short period of time
		// Passing an empty string will immediately clear the current string
		void AddEditorString(const std::string& str);

		struct PostProcessSettings
		{
			real saturation = 1.0f;
			glm::vec3 brightness;
			glm::vec3 offset;
			bool bEnableFXAA = true;
			bool bEnableFXAADEBUGShowEdges = false;
		};

		PostProcessSettings& GetPostProcessSettings();

		MaterialID GetPlaceholderMaterialID() const;

		void SetDisplayShadowCascadePreview(bool bPreview);
		bool GetDisplayShadowCascadePreview() const;

		bool IsTAAEnabled() const;

		i32 GetTAASampleCount() const;

		bool bFontWindowShowing = false;
		bool bUniformBufferWindowShowing = false;
		bool bGPUTimingsWindowShowing = false;

		static const u32 MAX_PARTICLE_COUNT = 65536;
		static const u32 PARTICLES_PER_DISPATCH = 256;
		static const u32 SSAO_NOISE_DIM = 4;

		static const char* GameObjectPayloadCStr;
		static const char* MaterialPayloadCStr;
		static const char* MeshPayloadCStr;

	protected:
		void LoadShaders();
		virtual bool LoadShaderCode(ShaderID shaderID) = 0;
		virtual void SetShaderCount(u32 shaderCount) = 0;
		virtual void RemoveMaterial(MaterialID materialID, bool bUpdateUsages = true) = 0;
		virtual void FillOutGBufferFrameBufferAttachments(std::vector<Pair<std::string, void*>>& outVec) = 0;
		virtual void RecreateShadowFrameBuffers() = 0;

		// Will attempt to find pre-rendered font at specified path, and
		// only render a new file if not present or if bForceRender is true
		// Returns true if succeeded
		virtual bool LoadFont(FontMetaData& fontMetaData, bool bForceRender) = 0;

		virtual void EnqueueScreenSpaceSprites();
		virtual void EnqueueWorldSpaceSprites();

		// If the object gets deleted this frame *gameObjectRef gets set to nullptr
		void DoCreateGameObjectButton(const char* buttonName, const char* popupName);

		// Returns true if the parent-child tree changed during this call
		bool DrawImGuiGameObjectNameAndChildren(GameObject* gameObject);

		void GenerateGBuffer();

		void EnqueueScreenSpaceText();
		void EnqueueWorldSpaceText();

		bool LoadFontMetrics(const std::vector<char>& fileMemory,
			FT_Library& ft,
			FontMetaData& metaData,
			std::map<i32, FontMetric*>* outCharacters,
			std::array<glm::vec2i, 4>* outMaxPositions,
			FT_Face* outFace);

		void InitializeMaterials();

		std::string PickRandomSkyboxTexture();

		u32 UpdateTextBufferSS(std::vector<TextVertex2D>& outTextVertices);
		u32 UpdateTextBufferWS(std::vector<TextVertex3D>& outTextVertices);

		glm::vec4 GetSelectedObjectColorMultiplier() const;
		glm::mat4 GetPostProcessingMatrix() const;

		void GenerateSSAONoise(std::vector<glm::vec4>& noise);

		MaterialID CreateParticleSystemSimulationMaterial(const std::string& name);
		MaterialID CreateParticleSystemRenderingMaterial(const std::string& name);

		void ParseFontFile();
		void SetRenderedSDFFilePath(FontMetaData& metaData);
		void SerializeFontFile();

		std::vector<Shader> m_BaseShaders;

		PointLightData* m_PointLights = nullptr;
		i32 m_NumPointLightsEnabled = 0;
		DirectionalLight* m_DirectionalLight = nullptr;

		struct DrawCallInfo
		{
			bool bRenderToCubemap = false;
			bool bDeferred = false;
			bool bWireframe = false;
			bool bWriteToDepth = true;
			bool bRenderingShadows = false;
			bool bCalculateDynamicUBOOffset = false;

			MaterialID materialIDOverride = InvalidMaterialID;

			u32 dynamicUBOOffset = 0;

			u64 graphicsPipelineOverride = InvalidID;
			u64 pipelineLayoutOverride = InvalidID;
			u64 descriptorSetOverride = InvalidID;
			Material::PushConstantBlock* pushConstantOverride = nullptr;

			// NOTE: DepthTestFunc only supported in GL, Vulkan requires specification at pipeline creation time
			DepthTestFunc depthTestFunc = DepthTestFunc::GEQUAL;
			RenderID cubemapObjectRenderID = InvalidRenderID;
			MaterialID materialOverride = InvalidMaterialID;
			CullFace cullFace = CullFace::_INVALID;
		};

		i32 m_ShadowCascadeCount = MAX_SHADOW_CASCADE_COUNT;
		u32 m_ShadowMapBaseResolution = 4096;

		std::vector<glm::mat4> m_ShadowLightViewMats;
		std::vector<glm::mat4> m_ShadowLightProjMats;

		// Filled every frame
		std::vector<SpriteQuadDrawInfo> m_QueuedWSSprites;
		std::vector<SpriteQuadDrawInfo> m_QueuedSSSprites;
		std::vector<SpriteQuadDrawInfo> m_QueuedSSArrSprites;

		// TODO: Use a mesh prefab here
		VertexBufferData m_Quad3DVertexBufferData;
		RenderID m_Quad3DRenderID = InvalidRenderID;
		RenderID m_Quad3DSSRenderID = InvalidRenderID;
		VertexBufferData m_FullScreenTriVertexBufferData;
		RenderID m_FullScreenTriRenderID = InvalidRenderID;

		RenderID m_GBufferQuadRenderID = InvalidRenderID;
		// Any editor objects which also require a game object wrapper
		std::vector<GameObject*> m_EditorObjects;

		bool m_bVSyncEnabled = true;
		PhysicsDebuggingSettings m_PhysicsDebuggingSettings;

		/* Objects that are created at bootup and stay active until shutdown, regardless of scene */
		std::vector<GameObject*> m_PersistentObjects;

		BitmapFont* m_CurrentFont = nullptr;
		// TODO: Separate fonts from font buffers
		std::vector<BitmapFont*> m_FontsSS;
		std::vector<BitmapFont*> m_FontsWS;

		PostProcessSettings m_PostProcessSettings;

		u32 m_FramesRendered = 0;

		bool m_bPostInitialized = false;
		bool m_bSwapChainNeedsRebuilding = false;
		bool m_bRebatchRenderObjects = true;

		bool m_bEnableWireframeOverlay = false;
		bool m_bEnableSelectionWireframe = false;
		bool m_bDisplayBoundingVolumes = false;
		bool m_bDisplayShadowCascadePreview = false;
		bool m_bRenderGrid = true;

		bool m_bCaptureScreenshot = false;
		bool m_bCaptureReflectionProbes = false;

		bool m_bShowEditorMaterials = false;

		bool m_bEnableTAA = true;
		i32 m_TAASampleCount = 2;
		bool m_bTAAStateChanged = false;

		i32 m_ShaderQualityLevel = 1;

		std::string m_PreviewedFont;

		GameObject* m_Grid = nullptr;
		GameObject* m_WorldOrigin = nullptr;
		MaterialID m_GridMaterialID = InvalidMaterialID;
		MaterialID m_WorldAxisMaterialID = InvalidMaterialID;

		GameObjectType m_NewObjectImGuiSelectedType = GameObjectType::OBJECT;

		sec m_EditorStrSecRemaining = 0.0f;
		sec m_EditorStrSecDuration = 1.5f;
		real m_EditorStrFadeDurationPercent = 0.25f;
		std::string m_EditorMessage;

		MaterialID m_ReflectionProbeMaterialID = InvalidMaterialID; // Set by the user via SetReflecionProbeMaterial

		MaterialID m_GBufferMaterialID = InvalidMaterialID;
		MaterialID m_SpriteMatSSID = InvalidMaterialID;
		MaterialID m_SpriteMatWSID = InvalidMaterialID;
		MaterialID m_SpriteArrMatID = InvalidMaterialID;
		MaterialID m_FontMatSSID = InvalidMaterialID;
		MaterialID m_FontMatWSID = InvalidMaterialID;
		MaterialID m_ShadowMaterialID = InvalidMaterialID;
		MaterialID m_PostProcessMatID = InvalidMaterialID;
		MaterialID m_PostFXAAMatID = InvalidMaterialID;
		MaterialID m_SelectedObjectMatID = InvalidMaterialID;
		MaterialID m_GammaCorrectMaterialID = InvalidMaterialID;
		MaterialID m_TAAResolveMaterialID = InvalidMaterialID;
		MaterialID m_PlaceholderMaterialID = InvalidMaterialID;
		MaterialID m_IrradianceMaterialID = InvalidMaterialID;
		MaterialID m_PrefilterMaterialID = InvalidMaterialID;
		MaterialID m_BRDFMaterialID = InvalidMaterialID;
		MaterialID m_WireframeMatID = InvalidMaterialID;

		MaterialID m_ComputeSDFMatID = InvalidMaterialID;
		MaterialID m_FullscreenBlitMatID = InvalidMaterialID;

		MaterialID m_SSAOMatID = InvalidMaterialID;
		MaterialID m_SSAOBlurMatID = InvalidMaterialID;
		ShaderID m_SSAOShaderID = InvalidShaderID;
		ShaderID m_SSAOBlurShaderID = InvalidShaderID;

		SpecializationConstantID m_SSAOKernelSizeSpecializationID = InvalidSpecializationConstantID;
		SpecializationConstantID m_TAASampleCountSpecializationID = InvalidSpecializationConstantID;
		SpecializationConstantID m_ShaderQualityLevelSpecializationID = InvalidSpecializationConstantID;
		SpecializationConstantID m_ShadowCascadeCountSpecializationID = InvalidSpecializationConstantID;

		std::string m_FontImageExtension = ".png";

		std::map<std::string, FontMetaData> m_Fonts;

		std::string m_RendererSettingsFilePathAbs;
		std::string m_FontsFilePathAbs;

		Mesh* m_SkyBoxMesh = nullptr;
		ShaderID m_SkyboxShaderID = InvalidShaderID;

		glm::mat4 m_LastFrameViewProj;

		// Contains file paths for each file with a .hdr extension in the `resources/textures/hdri/` directory
		std::vector<std::string> m_AvailableHDRIs;

		ShadowSamplingData m_ShadowSamplingData;

		Material::PushConstantBlock* m_SpritePerspPushConstBlock = nullptr;
		Material::PushConstantBlock* m_SpriteOrthoPushConstBlock = nullptr;
		Material::PushConstantBlock* m_SpriteOrthoArrPushConstBlock = nullptr;

		Material::PushConstantBlock* m_CascadedShadowMapPushConstantBlock = nullptr;

		// One per deferred-rendered shader
		ShaderBatch m_DeferredObjectBatches;
		// One per forward-rendered shader
		ShaderBatch m_ForwardObjectBatches;
		ShaderBatch m_ShadowBatch;

		ShaderBatch m_DepthAwareEditorObjBatches;
		ShaderBatch m_DepthUnawareEditorObjBatches;

		static const u32 NUM_GPU_TIMINGS = 64;
		std::vector<std::array<real, NUM_GPU_TIMINGS>> m_TimestampHistograms;
		u32 m_TimestampHistogramIndex = 0;

		TextureID m_AlphaBGTextureID = InvalidTextureID;
		TextureID m_LoadingTextureID = InvalidTextureID;
		TextureID m_WorkTextureID = InvalidTextureID;

		TextureID m_PointLightIconID = InvalidTextureID;
		TextureID m_DirectionalLightIconID = InvalidTextureID;

		SSAOGenData m_SSAOGenData;
		SSAOBlurDataConstant m_SSAOBlurDataConstant;
		SSAOBlurDataDynamic m_SSAOBlurDataDynamic;
		i32 m_SSAOKernelSize = MAX_SSAO_KERNEL_SIZE;
		i32 m_SSAOBlurSamplePixelOffset;
		SSAOSamplingData m_SSAOSamplingData;
		glm::vec2u m_SSAORes;
		bool m_bSSAOBlurEnabled = true;
		bool m_bSSAOStateChanged = false;

		FXAAData m_FXAAData;

		FT_Library m_FTLibrary;

		i32 m_DebugMode = 0;

		real m_TAA_ks[2];

		enum DirtyFlags : u32
		{
			CLEAN = 0,
			STATIC_DATA = 1 << 0,
			DYNAMIC_DATA = 1 << 1,
			SHADOW_DATA = 1 << 2,

			MAX_VALUE = 1 << 30,
			_NONE
		};

		u32 m_DirtyFlagBits = (u32)DirtyFlags::CLEAN;

		PhysicsDebugDrawBase* m_PhysicsDebugDrawer = nullptr;

		static std::array<glm::mat4, 6> s_CaptureViews;

		static const i32 LATEST_RENDERER_SETTINGS_FILE_VERSION = 1;
		i32 m_RendererSettingsFileVersion = 0;

		DirectoryWatcher* m_ShaderDirectoryWatcher = nullptr;

	private:
		Renderer& operator=(const Renderer&) = delete;
		Renderer(const Renderer&) = delete;

	};
} // namespace flex
