#include "stdafx.h"

#include "Scene/Prefabs/MeshPrefab.h"
#include "Logger.h"
#include "GameContext.h"
#include "Typedefs.h"
#include "Helpers.h"
#include "Colors.h"

#include <assimp/Importer.hpp>
#include <assimp/vector3.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

glm::vec4 MeshPrefab::m_DefaultColor(1.0f, 1.0f, 1.0f, 1.0f);
glm::vec3 MeshPrefab::m_DefaultPosition(0.0f, 0.0f, 0.0f);
glm::vec3 MeshPrefab::m_DefaultTangent(1.0f, 0.0f, 0.0f);
glm::vec3 MeshPrefab::m_DefaultBitangent(0.0f, 0.0f, 1.0f);
glm::vec3 MeshPrefab::m_DefaultNormal(0.0f, 1.0f, 0.0f);
glm::vec2 MeshPrefab::m_DefaultTexCoord(0.0f, 0.0f);

MeshPrefab::MeshPrefab()
{
}

MeshPrefab::~MeshPrefab()
{
	for (size_t i = 0; i < m_VertexBuffers.size(); i++)
	{
		m_VertexBuffers[i].Destroy();
	}
}

bool MeshPrefab::LoadFromFile(const GameContext& gameContext, const std::string& filepath)
{
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filepath,
		aiProcess_FindInvalidData |
		aiProcess_GenNormals
		);

	if (!pScene)
	{
		Logger::LogError(importer.GetErrorString());
		return false;
	}

	if (!pScene->HasMeshes())
	{
		Logger::LogWarning("Loaded mesh file has no meshes! " + filepath);
		return false;
	}

	aiMesh* mesh = pScene->mMeshes[0];

	m_Name = mesh->mName.C_Str();

	const size_t numVerts = mesh->mNumVertices;

	for (size_t i = 0; i < numVerts; i++)
	{
		// Position
		glm::vec3 pos = ToVec3(mesh->mVertices[i]);
		pos = glm::vec3(pos.x, pos.z, -pos.y); // Rotate +90 deg around x axis
		m_Positions.push_back(pos);
		m_HasElement |= (glm::uint)Renderer::VertexType::POSITION;

		// Color
		glm::vec4 col;
		if (mesh->HasVertexColors(0))
		{
			col = ToVec4(mesh->mColors[0][i]);
		}
		else
		{
			// Force color even on models which don't contain it
			col = m_DefaultColor;
		}
		m_Colors.push_back(col);
		m_HasElement |= (glm::uint)Renderer::VertexType::COLOR;

		// Tangent & Bitangent
		if (mesh->HasTangentsAndBitangents())
		{
			glm::vec3 tangent = ToVec3(mesh->mTangents[i]);
			m_Tangents.push_back(tangent);
			m_HasElement |= (glm::uint)Renderer::VertexType::TANGENT;

			glm::vec3 bitangent = ToVec3(mesh->mBitangents[i]);
			m_Bitangents.push_back(bitangent);
			m_HasElement |= (glm::uint)Renderer::VertexType::BITANGENT;
		}

		// Normal
		if (mesh->HasNormals())
		{
			glm::vec3 norm = ToVec3(mesh->mNormals[i]);
			m_Normals.push_back(norm);
			m_HasElement |= (glm::uint)Renderer::VertexType::NORMAL;
		}

		// TexCoord
		if (mesh->HasTextureCoords(0))
		{
			// Truncate w component
			glm::vec2 texCoord = (glm::vec2)(ToVec3(mesh->mTextureCoords[0][i]));
			m_TexCoords.push_back(texCoord);
			m_HasElement |= (glm::uint)Renderer::VertexType::TEXCOORD;
		}
	}

	Renderer* renderer = gameContext.renderer;

	m_VertexBuffers.push_back({});
	VertexBufferData* vertexBufferData = m_VertexBuffers.data() + (m_VertexBuffers.size() - 1);

	CreateVertexBuffer(vertexBufferData);

	Renderer::RenderObjectCreateInfo createInfo = {};
	createInfo.vertexBufferData = vertexBufferData;
	createInfo.diffuseMapPath = "resources/textures/brick_d.png";
	createInfo.specularMapPath = "resources/textures/brick_s.png";
	createInfo.normalMapPath = "resources/textures/brick_n.png";

	m_RenderID = renderer->Initialize(gameContext, &createInfo);
	
	renderer->SetTopologyMode(m_RenderID, Renderer::TopologyMode::TRIANGLE_LIST);

	DescribeShaderVariables(gameContext, vertexBufferData);
	
	return true;
}

bool MeshPrefab::LoadPrefabShape(const GameContext& gameContext, PrefabShape shape)
{
	Renderer* renderer = gameContext.renderer;

	m_VertexBuffers.push_back({});
	VertexBufferData* vertexBufferData = m_VertexBuffers.data() + (m_VertexBuffers.size() - 1);
	Renderer::RenderObjectCreateInfo createInfo = {};

	Renderer::TopologyMode topologyMode = Renderer::TopologyMode::TRIANGLE_LIST;

	switch (shape)
	{
	case MeshPrefab::PrefabShape::CUBE:
	{
		createInfo.diffuseMapPath = "resources/textures/brick_d.png";
		createInfo.specularMapPath = "resources/textures/brick_s.png";
		createInfo.normalMapPath = "resources/textures/brick_n.png";

		const std::array<glm::vec4, 6> colors = { Color::RED, Color::RED, Color::RED, Color::RED, Color::RED, Color::RED };
		m_Positions =
		{
			// Front
			{ -0.5f, -0.5f, -0.5f, },
			{ -0.5f,  0.5f, -0.5f, },
			{ 0.5f,  0.5f, -0.5f, },

			{ -0.5f, -0.5f, -0.5f, },
			{ 0.5f,  0.5f, -0.5f, },
			{ 0.5f, -0.5f, -0.5f, },

			// Top
			{ -0.5f, 0.5f, -0.5f, },
			{ -0.5f, 0.5f,  0.5f, },
			{ 0.5f,  0.5f,  0.5f, },

			{ -0.5f, 0.5f, -0.5f, },
			{ 0.5f,  0.5f,  0.5f, },
			{ 0.5f,  0.5f, -0.5f, },

			// Back
			{ 0.5f, -0.5f, 0.5f, },
			{ 0.5f,  0.5f, 0.5f, },
			{ -0.5f,  0.5f, 0.5f, },

			{ 0.5f, -0.5f, 0.5f, },
			{ -0.5f, 0.5f, 0.5f, },
			{ -0.5f, -0.5f, 0.5f, },

			// Bottom
			{ -0.5f, -0.5f, -0.5f, },
			{ 0.5f, -0.5f, -0.5f, },
			{ 0.5f,  -0.5f,  0.5f, },

			{ -0.5f, -0.5f, -0.5f, },
			{ 0.5f, -0.5f,  0.5f, },
			{ -0.5f, -0.5f,  0.5f, },

			// Right
			{ 0.5f, -0.5f, -0.5f, },
			{ 0.5f,  0.5f, -0.5f, },
			{ 0.5f,  0.5f,  0.5f, },

			{ 0.5f, -0.5f, -0.5f, },
			{ 0.5f,  0.5f,  0.5f, },
			{ 0.5f, -0.5f,  0.5f, },

			// Left
			{ -0.5f, -0.5f, -0.5f, },
			{ -0.5f,  0.5f,  0.5f, },
			{ -0.5f,  0.5f, -0.5f, },

			{ -0.5f, -0.5f, -0.5f, },
			{ -0.5f, -0.5f,  0.5f, },
			{ -0.5f,  0.5f,  0.5f, },
		};
		m_HasElement |= (glm::uint)Renderer::VertexType::POSITION;

		m_Colors =
		{
			// Front
			colors[0],
			colors[0],
			colors[0],

			colors[0],
			colors[0],
			colors[0],

			// Top
			colors[1],
			colors[1],
			colors[1],

			colors[1],
			colors[1],
			colors[1],

			// Back
			colors[2],
			colors[2],
			colors[2],

			colors[2],
			colors[2],
			colors[2],

			// Bottom
			colors[3],
			colors[3],
			colors[3],

			colors[3],
			colors[3],
			colors[3],

			// Right
			colors[4],
			colors[4],
			colors[4],

			colors[4],
			colors[4],
			colors[4],

			// Left
			colors[5],
			colors[5],
			colors[5],

			colors[5],
			colors[5],
			colors[5],
		};
		m_HasElement |= (glm::uint)Renderer::VertexType::COLOR;

		m_Normals =
		{
			// Front
			{ 0.0f, 0.0f, -1.0f, },
			{ 0.0f, 0.0f, -1.0f, },
			{ 0.0f, 0.0f, -1.0f, },

			{ 0.0f, 0.0f, -1.0f, },
			{ 0.0f, 0.0f, -1.0f, },
			{ 0.0f, 0.0f, -1.0f, },

			// Top
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },

			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },

			// Back
			{ 0.0f, 0.0f, 1.0f, },
			{ 0.0f, 0.0f, 1.0f, },
			{ 0.0f, 0.0f, 1.0f, },

			{ 0.0f, 0.0f, 1.0f, },
			{ 0.0f, 0.0f, 1.0f, },
			{ 0.0f, 0.0f, 1.0f, },

			// Bottom
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },

			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },
			{ 0.0f, -1.0f, 0.0f, },

			// Right
			{ 1.0f, 0.0f, 0.0f, },
			{ 1.0f, 0.0f, 0.0f, },
			{ 1.0f, 0.0f, 0.0f, },

			{ 1.0f, 0.0f, 0.0f, },
			{ 1.0f, 0.0f, 0.0f, },
			{ 1.0f, 0.0f, 0.0f, },

			// Left
			{ -1.0f, 0.0f, 0.0f, },
			{ -1.0f, 0.0f, 0.0f, },
			{ -1.0f, 0.0f, 0.0f, },

			{ -1.0f, 0.0f, 0.0f, },
			{ -1.0f, 0.0f, 0.0f, },
			{ -1.0f, 0.0f, 0.0f, },
		};
		m_HasElement |= (glm::uint)Renderer::VertexType::NORMAL;

		m_TexCoords =
		{
			// Front
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },

			// Top
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },

			// Back
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },

			// Bottom
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },

			// Right
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },

			// Left
			{ 0.0f, 0.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 0.0f, 0.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 0.0f },
		};
		m_HasElement |= (glm::uint)Renderer::VertexType::TEXCOORD;

	} break;
	case MeshPrefab::PrefabShape::ICO_SPHERE:
	{
		/*
		const float t = (1.0f + sqrt(5.0f)) / 2.0f;
		m_Positions = 
		{
			{ -1.0f, t, 0 },
			{ 1.0f, t, 0 },
			{ -1.0f, -t, 0 },
			{ 1.0f, -t, 0 },

			{ 0, -1.0f, t },
			{ 0, 1.0f, t },
			{ 0, -1.0f, -t },
			{ 0, 1.0f, -t },

			{ t, 0, -1.0f },
			{ t, 0, 1.0f },
			{ -t, 0, -1.0f },
			{ -t, 0, 1.0f }
		};
		m_HasElement |= (glm::uint)ILSemantic::POSITION;

		// TODO: Normals
		m_Normals = 
		{
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },

			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },

			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f },
			{ 0.0f, 0.0f, 1.0f }
		};
		m_HasElement |= (glm::uint)ILSemantic::NORMAL;

		m_Colors = 
		{
			Color::RED,
			Color::YELLOW,
			Color::GRAY,
			Color::ORANGE,

			Color::PINK,
			Color::WHITE,
			Color::PURPLE,
			Color::LIGHT_GRAY,

			Color::PURPLE,
			Color::LIGHT_BLUE,
			Color::LIME_GREEN,
			Color::LIGHT_GRAY
		};
		m_HasElement |= (glm::uint)ILSemantic::COLOR;

		// TODO: Proper tex coords
		m_TexCoords = 
		{
			{ 1.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 1.0f, 0.0f },
			{ 0.0f, 0.0f },

			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 0.0f, 1.0f },
			{ 1.0f, 1.0f },

			{ 1.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 1.0f },
			{ 1.0f, 1.0f }
		};
		m_HasElement |= (glm::uint)ILSemantic::TEXCOORD;

		m_Indices =
		{
			// 5 faces around point 0
			0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,

			// 5 adjacent faces
			1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,

			// 5 faces around point 3
			3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,

			// 5 adjacent faces
			4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1
		};

		const int iterationCount = 2;

		for (size_t i = 0; i < iterationCount; ++i)
		{
			const size_t numFaces = m_Indices.size() / 3;
			for (size_t j = 0; j < numFaces; ++j)
			{
				// Get three midpoints
				glm::vec3 midPointA = normalize(Lerp(m_Positions[m_Indices[j * 3]], m_Positions[m_Indices[j * 3 + 1]], 0.5f));
				glm::vec3 midPointB = normalize(Lerp(m_Positions[m_Indices[j * 3 + 1]], m_Positions[m_Indices[j * 3 + 2]], 0.5f));
				glm::vec3 midPointC = normalize(Lerp(m_Positions[m_Indices[j * 3 + 2]], m_Positions[m_Indices[j * 3]], 0.5f));

				m_Positions.push_back({ midPointA });
				m_Positions.push_back({ midPointB });
				m_Positions.push_back({ midPointC });

				m_Colors.push_back(Color::WHITE);
				m_Colors.push_back(Color::WHITE);
				m_Colors.push_back(Color::WHITE);

				m_TexCoords.push_back({ 0.5f, 0.5f });
				m_TexCoords.push_back({ 0.5f, 0.5f });
				m_TexCoords.push_back({ 0.5f, 0.5f });

				m_Normals.push_back({ 0.0f, 0.0f, 1.0f });
				m_Normals.push_back({ 0.0f, 0.0f, 1.0f });
				m_Normals.push_back({ 0.0f, 0.0f, 1.0f });

				int a = m_Positions.size() - 3;
				int b = m_Positions.size() - 2;
				int c = m_Positions.size() - 1;

				// TODO: Finish implementing



			}
		}
		*/
	} break;
	case MeshPrefab::PrefabShape::UV_SPHERE:
	{
		// Vertices
		glm::vec3 v1(0.0f, 1.0f, 0.0f); // Top vertex
		m_Positions.push_back(v1);
		m_Colors.push_back(Color::RED);
		m_TexCoords.push_back({ 0.0f, 0.0f });
		m_Normals.push_back({ 0.0f, 1.0f, 0.0f });

		m_HasElement |= (glm::uint)Renderer::VertexType::POSITION;
		m_HasElement |= (glm::uint)Renderer::VertexType::COLOR;
		m_HasElement |= (glm::uint)Renderer::VertexType::TEXCOORD;
		m_HasElement |= (glm::uint)Renderer::VertexType::NORMAL;

		glm::uint parallelCount = 10;
		glm::uint meridianCount = 5;

		assert(parallelCount > 0 && meridianCount > 0);

		const float PI = glm::pi<float>();
		const float TWO_PI = glm::two_pi<float>();

		for (glm::uint j = 0; j < parallelCount - 1; j++)
		{
			float polar = PI * float(j + 1) / (float)parallelCount;
			float sinP = sin(polar);
			float cosP = cos(polar);
			for (glm::uint i = 0; i < meridianCount; i++)
			{
				float azimuth = TWO_PI * (float)i / (float)meridianCount;
				float sinA = sin(azimuth);
				float cosA = cos(azimuth);
				glm::vec3 point(sinP * cosA, cosP, sinP * sinA);
				const glm::vec4 color =
					(i % 2 == 0 ? j % 2 == 0 ? Color::ORANGE : Color::PURPLE : j % 2 == 0 ? Color::WHITE : Color::YELLOW);

				m_Positions.push_back(point);
				m_Colors.push_back(color);
				m_TexCoords.push_back({ 0.0f, 0.0f });
				m_Normals.push_back({ 1.0f, 0.0f, 0.0f });
			}
		}

		glm::vec3 vF(0.0f, -1.0f, 0.0f); // Bottom vertex
		m_Positions.push_back(vF);
		m_Colors.push_back(Color::YELLOW);
		m_TexCoords.push_back({ 0.0f, 0.0f });
		m_Normals.push_back({ 0.0f, -1.0f, 0.0f });

		const glm::uint numVerts = m_Positions.size();


		// Indices
		m_Indices.clear();

		// Top triangles
		for (size_t i = 0; i < meridianCount; i++)
		{
			uint a = i + 1;
			uint b = (i + 1) % meridianCount + 1;
			m_Indices.push_back(0);
			m_Indices.push_back(b);
			m_Indices.push_back(a);
		}

		// Center quads
		for (size_t j = 0; j < parallelCount - 2; j++)
		{
			uint aStart = j * meridianCount + 1;
			uint bStart = (j + 1) * meridianCount + 1;
			for (size_t i = 0; i < meridianCount; i++)
			{
				uint a = aStart + i;
				uint a1 = aStart + (i + 1) % meridianCount;
				uint b = bStart + i;
				uint b1 = bStart + (i + 1) % meridianCount;
				m_Indices.push_back(a);
				m_Indices.push_back(a1);
				m_Indices.push_back(b1);

				m_Indices.push_back(a);
				m_Indices.push_back(b1);
				m_Indices.push_back(b);
			}
		}

		// Bottom triangles
		for (size_t i = 0; i < meridianCount; i++)
		{
			uint a = i + meridianCount * (parallelCount - 2) + 1;
			uint b = (i + 1) % meridianCount + meridianCount * (parallelCount - 2) + 1;
			m_Indices.push_back(numVerts - 1);
			m_Indices.push_back(a);
			m_Indices.push_back(b);
		}
	} break;
	case MeshPrefab::PrefabShape::GRID:
	{
		topologyMode = Renderer::TopologyMode::LINE_LIST;

		float rowWidth = 10.0f;
		int lineCount = 15;

		const glm::vec4 lineColor = Color::GRAY;
		const glm::vec4 centerLineColor = Color::LIGHT_GRAY;

		const size_t vertexCount = lineCount * 2 * 2;
		m_Positions.reserve(vertexCount);
		m_Colors.reserve(vertexCount);
		m_TexCoords.reserve(vertexCount);
		m_Normals.reserve(vertexCount);

		m_HasElement |= (glm::uint)Renderer::VertexType::POSITION;
		m_HasElement |= (glm::uint)Renderer::VertexType::COLOR;

		float halfWidth = (rowWidth * (lineCount - 1)) / 2.0f;

		// Horizontal lines
		for (int i = 0; i < lineCount; ++i)
		{
			m_Positions.push_back({ i * rowWidth - halfWidth, 0.0f, -halfWidth });
			m_Positions.push_back({ i * rowWidth - halfWidth, 0.0f, halfWidth });

			const glm::vec4 color = (i == lineCount / 2 ? centerLineColor : lineColor);
			m_Colors.push_back(color);
			m_Colors.push_back(color);
		}

		// Vertical lines
		for (int i = 0; i < lineCount; ++i)
		{
			m_Positions.push_back({ -halfWidth, 0.0f, i * rowWidth - halfWidth });
			m_Positions.push_back({ halfWidth, 0.0f, i * rowWidth - halfWidth });

			const glm::vec4 color = (i == lineCount / 2 ? centerLineColor : lineColor);
			m_Colors.push_back(color);
			m_Colors.push_back(color);
		}
	} break;
	default:
	{
		Logger::LogWarning("Unhandled prefab shape passed to MeshPrefab::LoadPrefabShape: " + std::to_string((int)shape));
		return false;
	} break;
	}

	CreateVertexBuffer(vertexBufferData);
	createInfo.vertexBufferData = vertexBufferData;

	m_RenderID = renderer->Initialize(gameContext, &createInfo);

	renderer->SetTopologyMode(m_RenderID, topologyMode);
	DescribeShaderVariables(gameContext, vertexBufferData);
	
	return true;
}

void MeshPrefab::Initialize(const GameContext& gameContext)
{
}

void MeshPrefab::UpdateAndRender(const GameContext& gameContext)
{
	Renderer* renderer = gameContext.renderer;
	
	glm::mat4 model = m_Transform.GetModelMatrix();
	renderer->UpdateTransformMatrix(gameContext, m_RenderID, model);
	
	renderer->Draw(gameContext, m_RenderID);
}

void MeshPrefab::Destroy(const GameContext& gameContext)
{
	gameContext.renderer->Destroy(m_RenderID);
}

void MeshPrefab::SetTransform(const Transform& transform)
{
	m_Transform = transform;
}

Transform& MeshPrefab::GetTransform()
{
	return m_Transform;
}

void MeshPrefab::DescribeShaderVariables(const GameContext& gameContext, VertexBufferData* vertexBufferData)
{
	Renderer* renderer = gameContext.renderer;

	constexpr size_t vertexTypeCount = 6;
	std::string names[vertexTypeCount] = { "in_Position", "in_Color", "in_Tangent", "in_Bitangent", "in_Normal", "in_TexCoord" };
	size_t sizes[vertexTypeCount] =      { 3,             4,          3,            3,              3,           2             };

	float* currentLocation = (float*)0;
	for (size_t i = 0; i < vertexTypeCount; ++i)
	{
		Renderer::VertexType vertexType = Renderer::VertexType(1 << i);
		if (m_HasElement & (int)vertexType)
		{
			renderer->DescribeShaderVariable(m_RenderID, gameContext.program, names[i], sizes[i], Renderer::Type::FLOAT, false,
				vertexBufferData->VertexStride, currentLocation);
			currentLocation += sizes[i];
		}
	}
}

void MeshPrefab::CreateVertexBuffer(VertexBufferData* vertexBufferData)
{
	vertexBufferData->VertexCount = m_Positions.size();
	vertexBufferData->VertexStride = CalculateVertexBufferStride();
	vertexBufferData->BufferSize = vertexBufferData->VertexCount * vertexBufferData->VertexStride;

	const std::string errorMsg = "Unqeual number of vertex components in MeshPrefab!";
	if (!m_Colors.empty()) Logger::Assert(m_Colors.size() == vertexBufferData->VertexCount, errorMsg);
	if (!m_Tangents.empty()) Logger::Assert(m_Tangents.size() == vertexBufferData->VertexCount, errorMsg);
	if (!m_Bitangents.empty()) Logger::Assert(m_Bitangents.size() == vertexBufferData->VertexCount, errorMsg);
	if (!m_Normals.empty()) Logger::Assert(m_Normals.size() == vertexBufferData->VertexCount, errorMsg);
	if (!m_TexCoords.empty()) Logger::Assert(m_TexCoords.size() == vertexBufferData->VertexCount, errorMsg);

	void *pDataLocation = malloc(vertexBufferData->BufferSize);
	if (pDataLocation == nullptr)
	{
		Logger::LogWarning("MeshPrefab::LoadPrefabShape failed to allocate memory required for vertex buffer data");
		return;
	}

	vertexBufferData->pDataStart = pDataLocation;

	for (UINT i = 0; i < vertexBufferData->VertexCount; ++i)
	{
		if (m_HasElement & (glm::uint)Renderer::VertexType::POSITION)
		{
			memcpy(pDataLocation, &m_Positions[i], sizeof(glm::vec3));
			pDataLocation = (float*)pDataLocation + 3;
		}

		if (m_HasElement & (glm::uint)Renderer::VertexType::COLOR)
		{
			memcpy(pDataLocation, &m_Colors[i], sizeof(glm::vec4));
			pDataLocation = (float*)pDataLocation + 4;
		}

		if (m_HasElement & (glm::uint)Renderer::VertexType::TANGENT)
		{
			memcpy(pDataLocation, &m_Tangents[i], sizeof(glm::vec3));
			pDataLocation = (float*)pDataLocation + 3;
		}

		if (m_HasElement & (glm::uint)Renderer::VertexType::BITANGENT)
		{
			memcpy(pDataLocation, &m_Bitangents[i], sizeof(glm::vec3));
			pDataLocation = (float*)pDataLocation + 3;
		}

		if (m_HasElement & (glm::uint)Renderer::VertexType::NORMAL)
		{
			memcpy(pDataLocation, &m_Normals[i], sizeof(glm::vec3));
			pDataLocation = (float*)pDataLocation + 3;
		}

		if (m_HasElement & (glm::uint)Renderer::VertexType::TEXCOORD)
		{
			memcpy(pDataLocation, &m_TexCoords[i], sizeof(glm::vec2));
			pDataLocation = (float*)pDataLocation + 2;
		}
	}
}

glm::uint MeshPrefab::CalculateVertexBufferStride() const
{
	glm::uint stride = 0;

	if (m_HasElement & (glm::uint)Renderer::VertexType::POSITION) stride += sizeof(glm::vec3);
	if (m_HasElement & (glm::uint)Renderer::VertexType::COLOR) stride += sizeof(glm::vec4);
	if (m_HasElement & (glm::uint)Renderer::VertexType::TANGENT) stride += sizeof(glm::vec3);
	if (m_HasElement & (glm::uint)Renderer::VertexType::BITANGENT) stride += sizeof(glm::vec3);
	if (m_HasElement & (glm::uint)Renderer::VertexType::NORMAL) stride += sizeof(glm::vec3);
	if (m_HasElement & (glm::uint)Renderer::VertexType::TEXCOORD) stride += sizeof(glm::vec2);

	if (stride == 0)
	{
		Logger::LogWarning("MeshPrefab's vertex buffer stride is 0!");
	}

	return stride;
}
