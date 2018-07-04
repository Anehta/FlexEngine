
#include "stdafx.hpp"

#include "JSONParser.hpp"

#include <fstream>
#include <cwctype>

#include "Helpers.hpp"
#include "Logger.hpp"
#include "Scene/GameObject.hpp"

namespace flex
{
	bool JSONParser::Parse(const std::string& filePath, JSONObject& rootObject)
	{
		std::string fileContents;
		if (!ReadFile(filePath, fileContents, false))
		{
			Logger::LogError("Couldn't find JSON file: " + filePath);
			return false;
		}

		size_t firstBracket = fileContents.find('{');
		size_t lastBracket = fileContents.rfind('}');

		if (firstBracket == std::string::npos ||
			lastBracket == std::string::npos || 
			firstBracket > lastBracket)
		{
			Logger::LogError("Failed to parse JSON file. No valid bracket pairs found!");
		}

		bool inQuote = false;
		i32 i = 0;
		while (i < (i32)fileContents.size())
		{
			char currentChar = fileContents[i];
			if (currentChar == '\"')
			{
				inQuote = !inQuote;
				++i;
				continue;
			}
			else
			{
				if (!inQuote)
				{
					// Remove all single line comments
					if (currentChar == '/' && fileContents[i + 1] == '/')
					{
						size_t endLine = fileContents.find('\n', i + 2);
						if (endLine == std::string::npos)
						{
							endLine = fileContents.size();
						}

						fileContents.erase(i, endLine - i);
						continue;
					}
					// Remove all whitespace characters
					else if (isspace(currentChar))
					{
						fileContents.erase(i, 1);
						continue;
					}
				}
			}

			++i;
		}

		rootObject = {};

		i32 fileContentOffset = 0;
		bool parseSucceeded = true;
		bool parsing = true;
		while (parsing)
		{
			JSONField field;
			parsing = ParseField(fileContents, &fileContentOffset, field);
			rootObject.fields.push_back(field);

			parseSucceeded |= parsing;

			if (fileContentOffset == (i32)(fileContents.size() - 1))
			{
				// We've reached the end of the file!
				parsing = false;
			}
		}

		return parseSucceeded;
	}

	Transform JSONParser::ParseTransform(const JSONObject& transformObject)
	{
		std::string posStr = transformObject.GetString("position");
		std::string rotStr = transformObject.GetString("rotation");
		std::string scaleStr = transformObject.GetString("scale");

		glm::vec3 pos(0.0f);
		if (!posStr.empty())
		{
			pos = ParseVec3(posStr);
		}

		glm::vec3 rotEuler(0.0f);
		if (!rotStr.empty())
		{
			rotEuler = ParseVec3(rotStr);
		}

		glm::vec3 scale(1.0f);
		if (!scaleStr.empty())
		{
			scale = ParseVec3(scaleStr);
		}

		// Check we aren't getting garbage data in
#if _DEBUG
		if (IsNanOrInf(pos))
		{
			Logger::LogError("Read garbage value from transform pos in serialized scene file! Using default value instead");
			pos = glm::vec3(0.0f);
		}

		if (IsNanOrInf(rotEuler))
		{
			Logger::LogError("Read garbage value from transform rot in serialized scene file! Using default value instead");
			rotEuler = glm::vec3(0.0f);
		}

		if (IsNanOrInf(scale))
		{
			Logger::LogError("Read garbage value from transform scale in serialized scene file! Using default value instead");
			scale = glm::vec3(1.0f);
		}
#endif

		glm::quat rotQuat = glm::quat(rotEuler);

		return Transform(pos, rotQuat, scale);
	}

	bool JSONParser::SerializeTransform(Transform* transform, JSONField& outTransformField)
	{
		if (transform == nullptr)
		{
			return false;
		}

		outTransformField = {};
		outTransformField.label = "transform";

		JSONObject transformObject = {};

		glm::vec3 localPos = transform->GetLocalPosition();
		glm::quat localRotQuat = transform->GetLocalRotation();
		glm::vec3 localScale = transform->GetLocalScale();
		
		if (IsNanOrInf(localPos))
		{
			Logger::LogError("Attempted to serialize garbage value for " + transform->GetGameObject()->GetName() + "'s pos - writing default value instead");
			localPos = glm::vec3(0.0f);
		}

		if (IsNanOrInf(localRotQuat))
		{
			Logger::LogError("Attempted to serialize garbage value for " + transform->GetGameObject()->GetName() + "'s rot - writing default value instead");
			localRotQuat = glm::quat();
		}

		if (IsNanOrInf(localScale))
		{
			Logger::LogError("Attempted to serialize garbage value for " + transform->GetGameObject()->GetName() + "'s scale - writing default value instead");
			localScale = glm::vec3(1.0f);
		}

		glm::vec3 localRotEuler = glm::eulerAngles(localRotQuat);

		std::string posStr = Vec3ToString(localPos);
		std::string rotStr = Vec3ToString(localRotEuler);
		std::string scaleStr = Vec3ToString(localScale);

		transformObject.fields.push_back(JSONField("position", JSONValue(posStr)));
		transformObject.fields.push_back(JSONField("rotation", JSONValue(rotStr)));
		transformObject.fields.push_back(JSONField("scale", JSONValue(scaleStr)));

		outTransformField.value = JSONValue(transformObject);

		return true;
	}

	bool JSONParser::ParseObject(const std::string& fileContents, i32* offset, JSONObject& outObject)
	{
		i32 objectClosingBracket = MatchingBracket('{', fileContents, *offset);
		if (objectClosingBracket == -1)
		{
			Logger::LogError("Couldn't find matching bracket for '{'");
			return false;
		}

		bool parsing = true;
		while (parsing && *offset < objectClosingBracket)
		{
			JSONField field;
			parsing = ParseField(fileContents, offset, field);
			outObject.fields.push_back(field);
		}

		*offset = objectClosingBracket + 1;

		if (fileContents[*offset] == ',')
		{
			*offset += 1;
		}

		return true;
	}

	bool JSONParser::ParseField(const std::string& fileContents, i32* offset, JSONField& field)
	{
		size_t quoteStart = fileContents.find('\"', *offset);

		if (quoteStart == std::string::npos)
		{
			Logger::LogError("Couldn't find opening quote after offset " + std::to_string(*offset));
			return false;
		}

		size_t quoteEnd = fileContents.find('\"', quoteStart + 1);
		if (quoteEnd == std::string::npos)
		{
			Logger::LogError("Couldn't find closing quote after offset " + std::to_string(*offset));
			return false;
		}

		field.label = fileContents.substr(quoteStart + 1, quoteEnd - (quoteStart + 1));
		*offset = quoteEnd + 1;

		if (fileContents[quoteEnd + 1] != ':')
		{
			Logger::LogError("Invalidly formatted JSON file (':' must occur after a field label)");
			return false;
		}

		char valueFirstChar = fileContents[quoteEnd + 2];
		JSONValue::Type fieldType = JSONValue::TypeFromChar(valueFirstChar, fileContents.substr(quoteEnd + 3));

		switch (fieldType)
		{
		case JSONValue::Type::STRING:
		{
			size_t strQuoteStart = fileContents.find('\"', *offset);

			if (strQuoteStart == std::string::npos)
			{
				Logger::LogError("Couldn't find quote after offset " + std::to_string(*offset));
				return false;
			}

			size_t strQuoteEnd = fileContents.find('\"', strQuoteStart + 1);
			if (strQuoteEnd == std::string::npos)
			{
				Logger::LogError("Couldn't find end quote after offset " + std::to_string(*offset));
				return false;
			}

			std::string stringValue = fileContents.substr(strQuoteStart + 1, strQuoteEnd - (strQuoteStart + 1));
			field.value = JSONValue(stringValue);

			*offset = strQuoteEnd + 1;
		} break;
		case JSONValue::Type::INT:
		{
			size_t intStart = quoteEnd + 2;
			size_t nextNonAlphaNumeric = NextNonAlphaNumeric(fileContents, intStart);
			size_t intCharCount = nextNonAlphaNumeric - intStart;
			std::string intStr = fileContents.substr(intStart, intCharCount);
			i32 intValue = stoi(intStr);
			field.value = JSONValue(intValue);

			*offset = nextNonAlphaNumeric;
		} break;
		case JSONValue::Type::FLOAT:
		{
			size_t floatStart = quoteEnd + 2;
			size_t decimalIndex = fileContents.find('.', floatStart);
			size_t floatEnd = NextNonAlphaNumeric(fileContents, decimalIndex + 1);
			size_t floatCharCount = floatEnd - floatStart;
			std::string floatStr = fileContents.substr(floatStart, floatCharCount);
			real floatValue = stof(floatStr);
			field.value = JSONValue(floatValue);

			*offset = floatEnd;
		} break;
		case JSONValue::Type::BOOL:
		{
			bool boolValue = valueFirstChar == 't';
			field.value = JSONValue(boolValue);

			*offset = NextNonAlphaNumeric(fileContents, quoteEnd + 3);
		} break;
		case JSONValue::Type::OBJECT:
		{
			*offset = quoteEnd + 2;

			JSONObject object;
			ParseObject(fileContents, offset, object);

			field.value = JSONValue(object);
		} break;
		case JSONValue::Type::OBJECT_ARRAY:
		{
			std::vector<JSONObject> objects;

			*offset = quoteEnd + 2;

			i32 arrayClosingBracket = MatchingBracket('[', fileContents, *offset);
			if (arrayClosingBracket == -1)
			{
				Logger::LogError("Couldn't find matching bracket " + field.label + " (for '[' )");
				return false;
			}

			*offset += 1;

			while (*offset < arrayClosingBracket)
			{
				JSONObject object;
				ParseObject(fileContents, offset, object);

				objects.push_back(object);
			}

			*offset = arrayClosingBracket + 1;

			field.value = JSONValue(objects);
		} break;
		case JSONValue::Type::FIELD_ARRAY:
		{
			std::vector<JSONField> fields;
			// TODO:
			field.value = JSONValue(fields);
		} break;
		case JSONValue::Type::UNINITIALIZED:
		default:
		{
			size_t nextNonAlphaNumeric = NextNonAlphaNumeric(fileContents, *offset);
			Logger::LogError("Unhandled JSON value type: " + fileContents.substr(quoteEnd + 2, nextNonAlphaNumeric - (quoteEnd + 2)));
			*offset = -1;
			return false;
		} break;
		}

		return true;
	}

	i32 JSONParser::MatchingBracket(char openingBracket, const std::string& fileContents, i32 offset)
	{
		assert(fileContents[offset] == openingBracket);

		char closingBracket;
		if (openingBracket == '[')
		{
			closingBracket = ']';
		}
		else if (openingBracket == '{')
		{
			closingBracket = '}';
		}
		else if (openingBracket == '(')
		{
			closingBracket = ')';
		}
		else
		{
			Logger::LogError("Unhandled opening bracket type: " + std::to_string(openingBracket));
			return -1;
		}

		i32 openSquareBracketCount = 0; // [
		i32 openCurlyBracketCount = 0;  // {
		i32 openParenthesesCount = 0;   // (

		bool inQuotes = false;
		while (offset < (i32)fileContents.size())
		{
			char currentChar = fileContents[offset];

			if (currentChar == '\"')
			{
				inQuotes = !inQuotes;
			}
			
			if (!inQuotes)
			{
				if (currentChar == '[')
				{
					++openSquareBracketCount;
				}
				else if (currentChar == ']')
				{
					--openSquareBracketCount;
				}
				else if (currentChar == '{')
				{
					++openCurlyBracketCount;
				}
				else if (currentChar == '}')
				{
					--openCurlyBracketCount;
				}
				else if (currentChar == '(')
				{
					++openParenthesesCount;
				}
				else if (currentChar == ')')
				{
					--openParenthesesCount;
				}

				if (currentChar == closingBracket &&
					openSquareBracketCount == 0 &&
					openCurlyBracketCount == 0 &&
					openParenthesesCount == 0)
				{
					return offset;
				}
			}

			++offset;
		}

		return -1;
	}
} // namespace flex
