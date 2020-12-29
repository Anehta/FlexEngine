#pragma once

namespace flex
{

#define ROOT_LOCATION			"../../../FlexEngine/"
#define SAVED_LOCATION			"../../../FlexEngine/saved/"
#define RESOURCE_LOCATION		"../../../FlexEngine/resources/"


#define CONFIG_DIRECTORY			ROOT_LOCATION "config/"
#define COMMON_CONFIG_LOCATION		CONFIG_DIRECTORY "common.json"
#define WINDOW_CONFIG_LOCATION		CONFIG_DIRECTORY "window-settings.json"
#define INPUT_BINDINGS_LOCATION		CONFIG_DIRECTORY "input-bindings.json"
#define RENDERER_SETTINGS_LOCATION	CONFIG_DIRECTORY "renderer-settings.json"
#define FONT_DEFINITION_LOCATION	CONFIG_DIRECTORY "fonts.json"
#define IMGUI_INI_LOCATION			CONFIG_DIRECTORY "imgui.ini"
#define IMGUI_LOG_LOCATION			CONFIG_DIRECTORY "imgui.log"

#define SCENE_DEFAULT_DIRECTORY		RESOURCE_LOCATION "scenes/default/"
#define SCENE_SAVED_DIRECTORY		RESOURCE_LOCATION "scenes/saved/"
#define PREFAB_DIRECTORY			RESOURCE_LOCATION "scenes/prefabs/"

#define GAME_OBJECT_TYPES_LOCATION	RESOURCE_LOCATION "GameObjectTypes.txt"

#define MESH_DIRECTORY				RESOURCE_LOCATION "meshes/"
#define MATERIALS_FILE_LOCATION		RESOURCE_LOCATION "scenes/materials.json"

#define TEXTURE_DIRECTORY			RESOURCE_LOCATION "textures/"

#define SFX_DIRECTORY				RESOURCE_LOCATION "audio/"

#define APP_ICON_DIRECTORY			RESOURCE_LOCATION "icons/"

#define FONT_DIRECTORY				RESOURCE_LOCATION "fonts/"
#define FONT_SDF_DIRECTORY			SAVED_LOCATION "fonts/"

#define SHADER_SOURCE_DIRECTORY		RESOURCE_LOCATION "shaders/"
#define SPV_DIRECTORY				SAVED_LOCATION "spv/"
#define SHADER_CHECKSUM_LOCATION	SAVED_LOCATION "vk-shader-checksum.dat"

#define SCREENSHOT_DIRECTORY		SAVED_LOCATION "screenshots/"

} // namespace flex
