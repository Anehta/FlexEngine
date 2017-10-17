#include "stdafx.hpp"
#if COMPILE_OPEN_GL

#include "Graphics/GL/GLHelpers.hpp"

#include <sstream>
#include <fstream>

#include "stb_image.h"

#include "Helpers.hpp"

namespace flex
{
	namespace gl
	{
		GLFWimage LoadGLFWimage(const std::string& filePath)
		{
			GLFWimage result = {};

			std::string fileName = filePath;
			StripLeadingDirectories(fileName);
			Logger::LogInfo("Loading texture " + fileName);

			int channels;
			unsigned char* data = stbi_load(filePath.c_str(), &result.width, &result.height, &channels, STBI_rgb);

			if (data == 0)
			{
				const char* failureReasonStr = stbi_failure_reason();
				Logger::LogError("Couldn't load image, failure reason: " + std::string(failureReasonStr) + " filepath: " + filePath);
				return result;
			}
			else
			{
				result.pixels = static_cast<unsigned char*>(data);
			}

			return result;
		}

		void DestroyGLFWimage(const GLFWimage& image)
		{
			stbi_image_free(image.pixels);
		}

		bool GenerateGLTexture_Empty(glm::uint& textureID, glm::vec2i dimensions, bool generateMipMaps, GLenum internalFormat, GLenum format, GLenum type)
		{
			return GenerateGLTexture_EmptyWithParams(textureID, dimensions, generateMipMaps, internalFormat, format, type, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
		}

		bool GenerateGLTexture_EmptyWithParams(glm::uint& textureID, glm::vec2i dimensions, bool generateMipMaps, GLenum internalFormat, GLenum format, GLenum type, int sWrap, int tWrap, int minFilter, int magFilter)
		{
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			CheckGLErrorMessages();

			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, dimensions.x, dimensions.y, 0, format, type, 0);
			CheckGLErrorMessages();
			if (generateMipMaps)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				CheckGLErrorMessages();
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			CheckGLErrorMessages();

			glBindTexture(GL_TEXTURE_2D, 0);

			glBindVertexArray(0);

			CheckGLErrorMessages();

			return true;
		}

		bool GenerateGLTexture(glm::uint& textureID, const std::string& filePath, bool generateMipMaps)
		{
			return GenerateGLTextureWithParams(textureID, filePath, generateMipMaps, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);
		}

		bool GenerateGLTextureWithParams(glm::uint& textureID, const std::string& filePath, bool generateMipMaps, int sWrap, int tWrap, int minFilter, int magFilter)
		{
			GLFWimage image = LoadGLFWimage(filePath);

			if (!image.pixels)
			{
				return false;
			}

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			CheckGLErrorMessages();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.pixels);
			CheckGLErrorMessages();
			if (generateMipMaps)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				CheckGLErrorMessages();
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			CheckGLErrorMessages();

			glBindTexture(GL_TEXTURE_2D, 0);

			glBindVertexArray(0);

			DestroyGLFWimage(image);

			CheckGLErrorMessages();

			return true;
		}

		bool GenerateHDRGLTexture(glm::uint& textureID, const std::string& filePath, bool generateMipMaps)
		{
			return GenerateHDRGLTextureWithParams(textureID, filePath, generateMipMaps, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
		}

		bool GenerateHDRGLTextureWithParams(glm::uint& textureID, const std::string& filePath, bool generateMipMaps, int sWrap, int tWrap, int minFilter, int magFilter)
		{
			HDRImage image = {};
			if (!image.Load(filePath, true))
			{
				return false;
			}

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			CheckGLErrorMessages();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, image.width, image.height, 0, GL_RGBA, GL_FLOAT, image.pixels);
			CheckGLErrorMessages();
			if (generateMipMaps)
			{
				glGenerateMipmap(GL_TEXTURE_2D);
				CheckGLErrorMessages();
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sWrap);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tWrap);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			CheckGLErrorMessages();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
			CheckGLErrorMessages();

			glBindTexture(GL_TEXTURE_2D, 0);

			image.Free();

			CheckGLErrorMessages();

			return true;
		}

		bool GenerateGLCubemapTextures(glm::uint& textureID, const std::array<std::string, 6> filePaths, bool generateMipmap)
		{
			bool success = true;

			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			CheckGLErrorMessages();

			for (size_t i = 0; i < filePaths.size(); ++i)
			{
				GLFWimage image = LoadGLFWimage(filePaths[i]);

				if (image.pixels)
				{
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, image.width, image.height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.pixels);
					CheckGLErrorMessages();

					DestroyGLFWimage(image);
				}
				else
				{
					Logger::LogError("Could not load cube map at " + filePaths[i]);
					success = false;
				}
			}

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			CheckGLErrorMessages();

			if (generateMipmap)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}

			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

			return success;
		}

		bool GenerateGLCubemap_Empty(glm::uint& textureID, int textureWidth, int textureHeight, bool generateMipmap, bool enableTrilinearFiltering)
		{
			glGenTextures(1, &textureID);
			glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
			CheckGLErrorMessages();

			for (unsigned int i = 0; i < 6; ++i)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
					textureWidth, textureHeight, 0, GL_RGB, GL_FLOAT, nullptr);
			}

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, (generateMipmap || enableTrilinearFiltering) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR); // Enable trilinear filtering when mip maps are enabled
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			CheckGLErrorMessages();

			if (generateMipmap)
			{
				glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			}

			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

			return true;
		}

		bool LoadGLShaders(glm::uint program, GLShader& shader)
		{
			CheckGLErrorMessages();

			bool success = true;

			GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
			CheckGLErrorMessages();

			GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
			CheckGLErrorMessages();

			std::string vertFileName = shader.shader.vertexShaderFilePath;
			StripLeadingDirectories(vertFileName);
			std::string fragFileName = shader.shader.fragmentShaderFilePath;
			StripLeadingDirectories(fragFileName);
			Logger::LogInfo("Loading shaders " + vertFileName + " & " + fragFileName);

			if (!ReadFile(shader.shader.vertexShaderFilePath, shader.shader.vertexShaderCode))
			{
				Logger::LogError("Could not find vertex shader " + shader.shader.name);
			}
			shader.shader.vertexShaderCode.push_back('\0'); // Signal end of string with terminator character

			if (!ReadFile(shader.shader.fragmentShaderFilePath, shader.shader.fragmentShaderCode))
			{
				Logger::LogError("Could not find fragment shader " + shader.shader.name);
			}
			shader.shader.fragmentShaderCode.push_back('\0'); // Signal end of string with terminator character

			GLint result = GL_FALSE;
			int infoLogLength;

			// Compile vertex shader
			char const* vertexSourcePointer = shader.shader.vertexShaderCode.data(); // TODO: Test
			glShaderSource(vertexShaderID, 1, &vertexSourcePointer, NULL);
			glCompileShader(vertexShaderID);

			glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
			if (result == GL_FALSE)
			{
				glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
				std::string vertexShaderErrorMessage;
				vertexShaderErrorMessage.resize((size_t)infoLogLength);
				glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, (GLchar*)vertexShaderErrorMessage.data());
				Logger::LogError(vertexShaderErrorMessage);
				success = false;
			}

			// Compile Fragment Shader
			char const* fragmentSourcePointer = shader.shader.fragmentShaderCode.data();
			glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
			glCompileShader(fragmentShaderID);

			glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
			if (result == GL_FALSE)
			{
				glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
				std::string fragmentShaderErrorMessage;
				fragmentShaderErrorMessage.resize((size_t)infoLogLength);
				glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, (GLchar*)fragmentShaderErrorMessage.data());
				Logger::LogError(fragmentShaderErrorMessage);
				success = false;
			}

			glAttachShader(program, vertexShaderID);
			glAttachShader(program, fragmentShaderID);
			CheckGLErrorMessages();

			return success;
		}

		bool LinkProgram(glm::uint program)
		{
			glLinkProgram(program);

			GLint result = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &result);
			if (result == GL_FALSE)
			{
				int infoLogLength;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
				std::string programErrorMessage;
				programErrorMessage.resize((size_t)infoLogLength);
				glGetProgramInfoLog(program, infoLogLength, NULL, (GLchar*)programErrorMessage.data());
				Logger::LogError(programErrorMessage);

				return false;
			}

			return true;
		}

		GLuint BufferTargetToGLTarget(Renderer::BufferTarget bufferTarget)
		{
			GLuint glTarget = 0;

			if (bufferTarget == Renderer::BufferTarget::ARRAY_BUFFER) glTarget = GL_ARRAY_BUFFER;
			else if (bufferTarget == Renderer::BufferTarget::ELEMENT_ARRAY_BUFFER) glTarget = GL_ELEMENT_ARRAY_BUFFER;
			else Logger::LogError("Unhandled BufferTarget passed to GLRenderer: " + std::to_string((int)bufferTarget));

			return glTarget;
		}

		GLenum TypeToGLType(Renderer::Type type)
		{
			GLenum glType = 0;

			if (type == Renderer::Type::BYTE) glType = GL_BYTE;
			else if (type == Renderer::Type::UNSIGNED_BYTE) glType = GL_UNSIGNED_BYTE;
			else if (type == Renderer::Type::SHORT) glType = GL_SHORT;
			else if (type == Renderer::Type::UNSIGNED_SHORT) glType = GL_UNSIGNED_SHORT;
			else if (type == Renderer::Type::INT) glType = GL_INT;
			else if (type == Renderer::Type::UNSIGNED_INT) glType = GL_UNSIGNED_INT;
			else if (type == Renderer::Type::FLOAT) glType = GL_FLOAT;
			else if (type == Renderer::Type::DOUBLE) glType = GL_DOUBLE;
			else Logger::LogError("Unhandled Type passed to GLRenderer: " + std::to_string((int)type));

			return glType;
		}

		GLenum UsageFlagToGLUsageFlag(Renderer::UsageFlag usage)
		{
			GLenum glUsage = 0;

			if (usage == Renderer::UsageFlag::STATIC_DRAW) glUsage = GL_STATIC_DRAW;
			else if (usage == Renderer::UsageFlag::DYNAMIC_DRAW) glUsage = GL_DYNAMIC_DRAW;
			else Logger::LogError("Unhandled usage flag passed to GLRenderer: " + std::to_string((int)usage));

			return glUsage;
		}

		GLenum TopologyModeToGLMode(Renderer::TopologyMode topology)
		{
			switch (topology)
			{
			case Renderer::TopologyMode::POINT_LIST: return GL_POINTS;
			case Renderer::TopologyMode::LINE_LIST: return GL_LINES;
			case Renderer::TopologyMode::LINE_LOOP: return GL_LINE_LOOP;
			case Renderer::TopologyMode::LINE_STRIP: return GL_LINE_STRIP;
			case Renderer::TopologyMode::TRIANGLE_LIST: return GL_TRIANGLES;
			case Renderer::TopologyMode::TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
			case Renderer::TopologyMode::TRIANGLE_FAN: return GL_TRIANGLE_FAN;
			default: return GL_INVALID_ENUM;
			}
		}

		glm::uint CullFaceToGLMode(Renderer::CullFace cullFace)
		{
			switch (cullFace)
			{
			case Renderer::CullFace::BACK: return GL_BACK;
			case Renderer::CullFace::FRONT: return GL_FRONT;
			case Renderer::CullFace::NONE: return GL_NONE; // TODO: This doesn't work, does it?
			default: return GL_FALSE;
			}
		}

		GLRenderObject::GLRenderObject(RenderID renderID, std::string name) :
			renderID(renderID)
		{
			info = {};
			info.name = name;
		}

		GLShader::GLShader(const std::string& name, const std::string& vertexShaderFilePath, const std::string& fragmentShaderFilePath) :
			shader(name, vertexShaderFilePath, fragmentShaderFilePath)
		{
		}
} // namespace gl
} // namespace flex

#endif // COMPILE_OPEN_GL