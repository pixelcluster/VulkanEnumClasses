//VulkanEnumClasses v1.0
//https://github.com/pixelcluster/VulkanEnumClasses

#include "generate.hpp"
#include "parsing_utils.hpp"
#include <iostream>

using namespace tinyxml2;
//defined for parsing_utils.hpp
unsigned int indentationLevel = 0;

void generateFromDocument(const XMLDocument& document, ParsingOptions& options, std::ostream& outStream)
{
	const XMLElement* registry = document.FirstChildElement("registry");
	if (!registry) {
		std::cout << "Error: Unable to find registry root node, is vk.xml valid?\n";
		return;
	}

	//Find extension tags if they need to be removed
	if (options.nameRemovePostfix || options.valueRemovePostfix || options.valueToLower) {
		const XMLElement* tagsElement = registry->FirstChildElement("tags");
		if (tagsElement) {
			const XMLElement* tag = tagsElement->FirstChildElement("tag");
			while (tag)
			{
				const char* tagName = tag->Attribute("name");
				if (tagName) options.extensionTagNames.push_back(tagName);
				tag = tag->NextSiblingElement("tag");
			}
		}
		if (options.extensionTagNames.empty()) {
			std::cout << "No extension tags found. Generation might not work flawlessly\n";
		}
	}

	EnumMap enums = parseBasicEnums(registry, options);
	includeFeatureEnums(registry, options, enums);
	includeExtensionEnums(registry, options, enums);
	writeEnums(enums, outStream, options);
}

EnumMap parseBasicEnums(const XMLElement* registry, const ParsingOptions& options)
{
	EnumMap enumMap;

	const XMLElement* enumsNode = registry->FirstChildElement("enums");
	if (!enumsNode) {
		std::cout << "Error: Unable to find enums nodes, is vk.xml valid?\n";
		return enumMap;
	}
	if (enumsNode->Attribute("name") && !strcmp(enumsNode->Attribute("name"), "API Constants")) {
		enumsNode = enumsNode->NextSiblingElement("enums");
	}
	while (enumsNode)
	{
		std::string name;
		const char* nameC = enumsNode->Attribute("name");
		if (nameC) {
			name = nameC;
		}
		enumMap.insert(std::pair<std::string, VulkanEnum>(name, parseBasicEnumNode(enumsNode, options)));
		enumsNode = enumsNode->NextSiblingElement("enums");
	}
	return enumMap;
}

void includeFeatureEnums(const XMLElement* registry, const ParsingOptions& options, EnumMap& enumMap)
{
	const XMLElement* featureNode = registry->FirstChildElement("feature");
	if (!featureNode) {
		std::cout << "Error: Unable to find feature nodes, is vk.xml valid?\n";
		return;
	}
	while (featureNode)
	{
		parseExtensionEnumNode(featureNode, options, enumMap);
		featureNode = featureNode->NextSiblingElement("feature");
	}
}

void includeExtensionEnums(const XMLElement* registry, const ParsingOptions& options, EnumMap& enumMap)
{
	const XMLElement* extensionsNode = registry->FirstChildElement("extensions");
	if (!extensionsNode) {
		std::cout << "Error: Unable to find extensions nodes, is vk.xml valid?\n";
		return;
	}
	std::unordered_set<std::string> extensionNames;

	if (options.excludeExtensions) {
		for (auto& includedName : options.includeExtensionList) {
			includeExtensionDependencies(extensionsNode, includedName, extensionNames);
		}
	}
	else {
		listExtensionNames(extensionsNode, extensionNames);
		for (auto& excludedName : options.excludeExtensionList) {
			excludeDependentExtensions(extensionsNode, excludedName, extensionNames);
		}
	}

	const XMLElement* extensionNode = extensionsNode->FirstChildElement("extension");
	while (extensionNode)
	{
		const char* name = extensionNode->Attribute("name");
		if (name) {
			if (extensionNames.find(name) != extensionNames.end())
				parseExtensionEnumNode(extensionNode, options, enumMap);
		}
		extensionNode = extensionNode->NextSiblingElement("extension");
	}
}

void writeEnums(const EnumMap& enumMap, std::ostream& outStream, const ParsingOptions& options)
{
	addLine(outStream, "#ifndef __VULKANENUMS_HPP");
	addLine(outStream, "#define __VULKANENUMS_HPP");

	addLine(outStream, "#include <cstdint>");

	addLine(outStream, "#ifdef _MSC_VER");
	addLine(outStream, "#pragma warning( disable : 4146 )"); //Disable sign on unsigned value warnings (triggered on "-1U")
	addLine(outStream, "#endif");
	//namespace opener
	if (options.useNamespaces) {
		addLine(outStream, "namespace " + options.namespaceName + " {");
		++indentationLevel;
	}

	for (const auto& enumPair : enumMap) {
		const VulkanEnum& enumValue = enumPair.second;
		if (enumValue.isIncluded) {

			std::string baseType;
			switch (enumValue.type) {
			case VulkanEnumType::Enum:
			case VulkanEnumType::Bitmask:
				baseType = " : uint32_t";
				break;
			case VulkanEnumType::Bitmask64:
				baseType = " : uint64_t";
				break;
			}

			addLine(outStream, "enum class " + enumValue.name + baseType + " {");
			++indentationLevel;

			const std::vector<VulkanEnumValue>& values = enumValue.values();
			const std::vector<VulkanEnumValue>& aliasValues = enumValue.aliasValues();
			for (size_t i = 0; i < values.size(); ++i) {
				writeEnumValue(values[i], outStream, i == (values.size() + aliasValues.size()) - 1);
			}
			for (size_t i = 0; i < aliasValues.size(); ++i) {
				writeEnumValue(aliasValues[i], outStream, i == aliasValues.size() - 1);
			}
			if (!values.size()) {
				addLine(outStream, "//Placeholder for an empty enum in the spec (empty enums are ill-formed in C++). Will be removed once the enum gets values.");
				addLine(outStream, "Empty");
			}

			--indentationLevel;
			addLine(outStream, "};");

			//Add logical operations for bitmask types
			if (enumValue.type != VulkanEnumType::Enum) {
				bool is64Bit = enumValue.type == VulkanEnumType::Bitmask64;

				addLine(outStream, "");
				addEnumOperator(outStream, enumValue.name, "|", is64Bit);
				addEnumOperator(outStream, enumValue.name, "&", is64Bit);
				addEnumOperator(outStream, enumValue.name, "^", is64Bit);

				//NOT (~) operator
				addLine(outStream, "inline " + enumValue.name + " operator~(" + enumValue.name + " obj) {");
				++indentationLevel;

				if(is64Bit)
					addLine(outStream, "return static_cast<" + enumValue.name + ">(~static_cast<uint64_t>(obj));");
				else
					addLine(outStream, "return static_cast<" + enumValue.name + ">(~static_cast<uint32_t>(obj));");

				--indentationLevel;
				addLine(outStream, "}");
			}
		}
	}

	if (options.useNamespaces) {
		--indentationLevel;
		addLine(outStream, "}");
	}
	addLine(outStream, "#endif");
}

void writeEnumValue(const VulkanEnumValue& enumValue, std::ostream& outStream, bool isLastValue)
{
	//Explicitly filter out aliases with the same name, avoiding redefinitions
	if(enumValue.name == enumValue.value) return;

	if (!enumValue.comment.empty()) {
		addLine(outStream, "//" + enumValue.comment);
	}

	std::string valueText;
	if (enumValue.isBitpos) {
		valueText = "1ULL << " + enumValue.value;
	}
	else {
		valueText = enumValue.value;

		//Add explicit unsignedness to negative values
		if(valueText.find('-') != std::string::npos) {
			valueText.push_back('U');
		}
	}
	addLine(outStream, enumValue.name + " = " + valueText + (isLastValue ? "" : ", "));
}

VulkanEnum parseBasicEnumNode(const XMLElement* node, const ParsingOptions& options)
{
	VulkanEnum result;

	const char* nameC = node->Attribute("name");
	if (nameC) {
		result.name = nameC;
		result.originalName = nameC;
	}
	processName(options, result.name);

	const char* type = node->Attribute("type");
	if (type && !strcmp(type, "bitmask")) {
		const char* bitwidth = node->Attribute("bitwidth");
		if (bitwidth && !strcmp(bitwidth, "64")) {
			result.type = VulkanEnumType::Bitmask64;
		}
		else {
			result.type = VulkanEnumType::Bitmask;
		}
	}

	const XMLElement* valueNode = node->FirstChildElement("enum");
	while (valueNode)
	{
		parseValueNode(valueNode, result, options);
		valueNode = valueNode->NextSiblingElement("enum");
	}
	return result;
}

void parseExtensionEnumNode(const XMLElement* node, const ParsingOptions& options, EnumMap& enums)
{
	const XMLElement* requireNode = node->FirstChildElement("require");
	while (requireNode)
	{
		const XMLElement* enumNode = requireNode->FirstChildElement("enum");
		while (enumNode)
		{
			const char* extendsName = enumNode->Attribute("extends");
			if (extendsName) {
				parseValueNode(enumNode, enums[extendsName], options);
			}
			enumNode = enumNode->NextSiblingElement("enum");
		}
		const XMLElement* typeNode = requireNode->FirstChildElement("type");
		while (typeNode)
		{
			const char* referencedName = typeNode->Attribute("name");
			if (referencedName) {
				auto enumIterator = enums.find(referencedName);
				if (enumIterator != enums.end()) {
					enumIterator->second.isIncluded = true;
				}
			}
			typeNode = typeNode->NextSiblingElement("type");
		}
		requireNode = requireNode->NextSiblingElement("require");
	}
}

void parseValueNode(const XMLElement* valueNode, VulkanEnum& vulkanEnum, const ParsingOptions& options)
{
	VulkanEnumValue enumValue;
	const XMLNode* parent = valueNode->Parent();
	const XMLNode* grandparent = parent ? parent->Parent() : nullptr;

	std::string valueName;
	const char* valueNameC = valueNode->Attribute("name");
	const char* valueC = valueNode->Attribute("value");
	const char* bitposC = valueNode->Attribute("bitpos");
	const char* aliasC = valueNode->Attribute("alias");
	const char* offsetC = valueNode->Attribute("offset");
	const char* extNumberC = grandparent ? grandparent->ToElement()->Attribute("number") : nullptr;
	const char* commentC = valueNode->Attribute("comment");
	const char* dirC = valueNode->Attribute("dir");

	if (commentC) {
		enumValue.comment = commentC;
	}

	if (valueNameC) {
		valueName = valueNameC;
		enumValue.name = valueName;
		processValueName(options, enumValue.name, vulkanEnum.originalName);
	}

	if (offsetC && extNumberC) {
		int offset = atoi(offsetC);
		int extNumber = atoi(extNumberC);
		enumValue.value = std::to_string(1000000000 + (extNumber - 1) * 1000 + offset);
	}
	else if (aliasC) {
		enumValue.value = aliasC;
		processValueName(options, enumValue.value, vulkanEnum.originalName);
		vulkanEnum.addAliasEnumValue(valueName, enumValue);
	}
	else if (bitposC) {
		enumValue.value = bitposC;
		enumValue.isBitpos = true;
	}
	else if (valueC) {
		enumValue.value = valueC;
	}

	if (dirC && dirC[0] == '-') {
		enumValue.value.insert(enumValue.value.begin(), '-');
	}
	vulkanEnum.addEnumValue(valueName, enumValue);

}

void processName(const ParsingOptions& options, std::string& name)
{
	//Change FlagBits to Flags since the actual bitmask is integrated into being an enum here
	size_t flagBitsIndex = name.find("FlagBits");
	if (flagBitsIndex != std::string::npos)
		name.replace(flagBitsIndex, 8, "Flags");

	if (name.find("Vk") == 0) {
		name.replace(0, 2, options.namePrefixReplacement);
	}
	if (options.nameRemovePostfix) {
		removeTags(options.extensionTagNames, name);
	}
}

void processValueName(const ParsingOptions& options, std::string& valueName, const std::string& originalStructureName, bool allowLeadingDigits)
{
	size_t prefixLength = valueName.find("VK_") == 0 ? 3 : 0;

	if (options.removeStructureNames) {
		//The original unprocessed name is needed
		std::string structureName = structureNameToEnumValue(originalStructureName);
		// The extension tag will be removed separately in the value, 
		// but it might interfere with values that start with the same letter as an extension tag
		// (e.g. Error and EXT)
		removeTags(options.extensionTagNames, structureName);
		//Add trailing underscore to remove the underscore connecting enum type and value
		structureName.push_back('_');
		//Remove all characters matching the structure name
		for (size_t i = 0; i < valueName.length(); ++i) {
			if (valueName[i] == structureName[i]) {
				valueName.erase(i, 1);
				structureName.erase(i, 1); //Needed because otherwise indices get out of sync
				--i; //Adjust index because of erasure
			}
			else break;
		}
		prefixLength = 0; //options.removeStructureNames already removes the prefix
	}
	else {
		valueName.erase(0, prefixLength);
	}
	valueName.insert(0, options.valuePrefixReplacement);
	prefixLength = options.valuePrefixReplacement.length();

	if (hasTags(options.extensionTagNames, originalStructureName)) {
		if (options.valueRemovePostfix) {
			removeTags(options.extensionTagNames, valueName, true);
		}
	}
	else {
		if (options.valueRemovePostfixOnCoreTypes) {
			removeTags(options.extensionTagNames, valueName, true);
		}
	}

	//Convert to lowercase
	if (options.valueToLower) {
		for (size_t i = prefixLength; i < valueName.length(); ++i) {
			valueName[i] = tolower(valueName[i]);
		}
	}

	if (options.valueCapitalizeStart) {
		valueName[prefixLength] = toupper(valueName[prefixLength]);
		for (size_t i = prefixLength + 1; i < valueName.length(); ++i) {
			if (valueName[i - 1] == '_' || isdigit(valueName[i - 1])) {
				valueName[i] = toupper(valueName[i]);
			}
		}
	}

	//Remove underscores
	if (options.removeUnderscores) {
		for (size_t i = prefixLength; i < valueName.length(); ++i) {
			if (valueName[i] == '_') {
				valueName.erase(i, 1);
				--i; //The current character got erased, so the index needs to be adjusted
			}
		}
	}

	if (!allowLeadingDigits && isdigit(valueName[0])) {
		valueName.insert(0, options.numberPrefix);
	}
}

void includeExtensionDependencies(const XMLElement* extensionsNode, const std::string& extensionName, std::unordered_set<std::string>& dependencyNames)
{
	std::unordered_set<std::string> dependencies;
	const XMLElement* extensionNode = extensionsNode->FirstChildElement("extension");
	while (extensionNode)
	{
		const char* name = extensionNode->Attribute("name");
		if (name && extensionName == name) {
			const char* dependenciesText = extensionNode->Attribute("requires");
			if (dependenciesText) {
				std::vector<std::string> dependencyExtensions = splitList(dependenciesText, ',');
				dependencies.insert(dependencyExtensions.begin(), dependencyExtensions.end());
			}
			break;
		}
		extensionNode = extensionNode->NextSiblingElement("extension");
	}

	for (auto& dependency : dependencies) {
		includeExtensionDependencies(extensionsNode, dependency, dependencies);
	}
	dependencyNames.insert(dependencies.begin(), dependencies.end());
}

void excludeDependentExtensions(const XMLElement* extensionsNode, const std::string& extensionName, std::unordered_set<std::string>& dependencyNames)
{
	std::unordered_set<std::string> dependentExtensions;
	const XMLElement* extensionNode = extensionsNode->FirstChildElement("extension");
	while (extensionNode)
	{
		const char* name = extensionNode->Attribute("name");
		if (name) {
			const char* dependentExtensionsText = extensionNode->Attribute("requires");

			if (dependentExtensionsText) {
				std::vector<std::string> dependencyExtensions = splitList(dependentExtensionsText, ',');

				for (auto& dependency : dependencyExtensions) {
					if (dependency == extensionName) {
						dependencyNames.erase(dependency);
						dependentExtensions.insert(dependency);
					}
				}
			}
		}
		extensionNode = extensionNode->NextSiblingElement("extension");
	}

	for (auto& dependency : dependentExtensions) {
		excludeDependentExtensions(extensionsNode, dependency, dependentExtensions);
	}
}

void listExtensionNames(const XMLElement* extensionsNode, std::unordered_set<std::string>& dependencyNames)
{
	const XMLElement* extensionNode = extensionsNode->FirstChildElement("extension");
	while (extensionNode)
	{
		const char* name = extensionNode->Attribute("name");
		if (name) {
			dependencyNames.insert(name);
		}
		extensionNode = extensionNode->NextSiblingElement("extension");
	}
}
