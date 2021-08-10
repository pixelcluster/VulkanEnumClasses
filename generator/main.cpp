//VulkanEnumClasses v0.9
//https://github.com/pixelcluster/VulkanEnumClasses


#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include "generate.hpp"
#include "parsing_utils.hpp"

struct Argument
{
	std::string name;
	std::string additionalData;
};

std::vector<Argument> parseArguments(int argc, char** argv) {
	Argument currentArgument;
	std::vector<Argument> arguments;
	bool hasWrittenArguments = false;

	for (int i = 1; i < argc; ++i) {
		hasWrittenArguments = true;
		bool isNewArgument = false;
		while (*argv[i] == '-')
		{
			++argv[i];
			isNewArgument = true;
		}

		if (isNewArgument) {
			arguments.push_back(currentArgument);
			currentArgument = Argument();
			currentArgument.name = argv[i];
		}
		else {
			if (!currentArgument.additionalData.empty()) currentArgument.additionalData += " ";
			currentArgument.additionalData += argv[i];
		}
	}
	if(hasWrittenArguments)
		arguments.push_back(currentArgument);
	return arguments;
}

int main(int argc, char** argv) {
	std::string xmlPath;

	ParsingOptions options;

	std::vector<Argument> arguments = parseArguments(argc, argv);
	for (auto& argument : arguments) {
		if (argument.name.empty()) continue;
		else if (argument.name == "path") {
			xmlPath = argument.additionalData;
		}
		else if (argument.name == "namespace") {
			options.useNamespaces = true;
			options.namespaceName = argument.additionalData;
		}
		else if (argument.name == "exclude-extensions") {
			options.excludeExtensions = true;
		}
		else if (argument.name == "include") {
			options.includeExtensionList = splitList(argument.additionalData);
		}
		else if (argument.name == "exclude") {
			options.excludeExtensionList = splitList(argument.additionalData);
		}
		else if (argument.name == "replace-names") {
			options.replaceNames = true;
		}
		else if (argument.name == "name-prefix-replacement") {
			options.namePrefixReplacement = argument.additionalData;
		}
		else if (argument.name == "name-remove-postfix") {
			options.nameRemovePostfix = true;
		}
		else if (argument.name == "replace-values") {
			options.replaceValues = true;
		}
		else if (argument.name == "value-prefix-replacement") {
			options.valuePrefixReplacement = argument.additionalData;
		}
		else if (argument.name == "value-number-prefix") {
			options.numberPrefix = argument.additionalData;
		}
		else if (argument.name == "remove-structure-names") {
			options.removeStructureNames = true;
		}
		else if (argument.name == "remove-underscores") {
			options.removeUnderscores = true;
		}
		else if (argument.name == "tolower") {
			options.valueToLower = true;
		}
		else if (argument.name == "capitalize-start") {
			options.valueCapitalizeStart = true;
		}
		else if (argument.name == "value-remove-postfix") {
			options.valueRemovePostfix = true;
		}
		else if (argument.name == "value-remove-postfix-core-types") {
			options.valueRemovePostfixOnCoreTypes = true;
		}
		else {
			std::cout << "Warning: Unrecognized argument " << argument.name << std::endl;
		}
	}

	tinyxml2::XMLDocument vkXml;
	vkXml.LoadFile(xmlPath.c_str());
	if (vkXml.Error()) {
		std::cout << "Error: Error opening or parsing" << xmlPath <<  "! Does the file exist?\n";
		return EXIT_FAILURE;
	}

	std::ofstream cppFile = std::ofstream("VulkanEnums.hpp", std::ios::trunc);
	if (!cppFile.is_open()) {
		std::cout << "Error: Error opening the output file! Is it in use?\n";
		return EXIT_FAILURE;
	}
	generateFromDocument(vkXml, options, cppFile);
}