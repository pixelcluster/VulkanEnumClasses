//VulkanEnumClasses v1.0
//https://github.com/pixelcluster/VulkanEnumClasses

#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <ostream>
#include <tinyxml2.h>
#include <unordered_set>

//Main header generation logic.
//Generates a C++ header based on the Vulkan vk.xml specification file.

//User-defined options on what exactly to parse and how to generate the header
struct ParsingOptions
{
	bool useNamespaces = false;
	std::string namespaceName;

	bool excludeExtensions = false;
	std::vector<std::string> includeExtensionList;
	std::vector<std::string> excludeExtensionList;

	bool replaceNames = false;
	std::string namePrefixReplacement;
	bool nameRemovePostfix = false;

	bool replaceValues = false;
	std::string valuePrefixReplacement;
	std::string numberPrefix;
	bool removeUnderscores = false;
	bool removeStructureNames = false;
	bool valueToLower = false;
	bool valueCapitalizeStart = false;
	bool valueRemovePostfix = false;
	bool valueRemovePostfixOnCoreTypes = false;
	std::vector<std::string> extensionTagNames;
};

struct VulkanEnumValue
{
	std::string name, value, comment;
	bool isBitpos = false;
};

class VulkanEnum
{
public:
	std::string name;
	std::string originalName;
	bool isIncluded = false;
	bool isBitmask = false;

	//Enum value definitions are sometimes duplicated across vk.xml, this makes sure the same value name won't occur twice
	void addEnumValue(std::string originalValueName, VulkanEnumValue value) {
		if (m_originalValueNames.find(originalValueName) == m_originalValueNames.end()) {
			m_values.push_back(value);
			m_originalValueNames.insert(originalValueName);
		}
	}

	void addAliasEnumValue(std::string originalValueName, VulkanEnumValue value) {
		if (m_originalValueNames.find(originalValueName) == m_originalValueNames.end()) {
			m_aliasValues.push_back(value);
			m_originalValueNames.insert(originalValueName);
		}
	}

	const std::vector<VulkanEnumValue>& values() const {
		return m_values;
	}

	const std::vector<VulkanEnumValue>& aliasValues() const {
		return m_aliasValues;
	}
private:
	std::vector<VulkanEnumValue> m_values;
	//Alias values need to be put last in order to ensure the aliases are already defined
	std::vector<VulkanEnumValue> m_aliasValues;
	std::unordered_set<std::string> m_originalValueNames;
};

namespace std {
	template<>
	struct hash<VulkanEnum> {
		size_t operator()(const VulkanEnum& enumValue) const {
			return hash<string>()(enumValue.originalName);
		}
	};
}

using EnumMap = std::unordered_map<std::string, ::VulkanEnum>;

//Generates a full header file from document according to the parsing options and writes it to outStream.
void generateFromDocument(const tinyxml2::XMLDocument& document, ParsingOptions& options, std::ostream& outStream);

//Document/Structure helpers

//Look up all defined enums
EnumMap parseBasicEnums(const tinyxml2::XMLElement* registry, const ParsingOptions& options);
//Finds and includes all feature enums (=core enums that were added in later versions) and applies additions to existing enums.
void includeFeatureEnums(const tinyxml2::XMLElement* registry, const ParsingOptions& options, EnumMap& enums);
//Finds and includes all extension enums that are specified in the parsing options and applies additions to existing enums.
void includeExtensionEnums(const tinyxml2::XMLElement* registry, const ParsingOptions& options, EnumMap& enums);
//Writes the included enums to a stream
void writeEnums(const EnumMap& enums, std::ostream& outStream, const ParsingOptions& options);
//Writes one enum value to a stream
void writeEnumValue(const VulkanEnumValue& enumValue, std::ostream& outStream, bool isLastValue);

//Node helpers

//Parses a basic enum node which isn't an extension to other existing nodes
VulkanEnum parseBasicEnumNode(const tinyxml2::XMLElement* node, const ParsingOptions& options);
//Parses an extension node. Includes referenced enums and applies additions.
void parseExtensionEnumNode(const tinyxml2::XMLElement* node, const ParsingOptions& options, EnumMap& enums);

//Name/Value helpers

//Parses an XML node containing an enum value
void parseValueNode(const tinyxml2::XMLElement* node, VulkanEnum& vulkanEnum, const ParsingOptions& options);
//Processes an enum name from its vk.xml form to the desired header form
void processName(const ParsingOptions& options, std::string& name);
//Processes an enum value name from its vk.xml form to the desired header form
void processValueName(const ParsingOptions& options, std::string& valueName, const std::string& originalStructureName, bool allowLeadingDigits = false);

//Extension (dependency) helpers

//For an <extension> node given by extensionNode, this function adds all extensions that are required by extensionNode to dependencyNames.
//dependencyNames should be empty.
void includeExtensionDependencies(const tinyxml2::XMLElement* extensionsNode, const std::string& extensionName, std::unordered_set<std::string>& dependencyNames);
//This function removes all extensions that depend on the extension named by extensionName from dependencyNames.
//The extensionsNode parameter does NOT specify the <extension> node, but its parent <extensions> node.
void excludeDependentExtensions(const tinyxml2::XMLElement* extensionsNode, const std::string& extensionName, std::unordered_set<std::string>& dependencyNames);
//This function lists all extensions.
//The extensionsNode parameter does NOT specify the <extension> node, but its parent <extensions> node.
void listExtensionNames(const tinyxml2::XMLElement* extensionsNode, std::unordered_set<std::string>& dependencyNames);