//VulkanEnumClasses v0.9
//https://github.com/pixelcluster/VulkanEnumClasses

#pragma once
#include <vector>
#include <string>
#include <ostream>
#include <sstream>

//defined in generate.cpp
extern unsigned int indentationLevel;

inline std::vector<std::string> splitList(const std::string& list, char separator = ' ') {
	std::stringstream listStream = std::stringstream(list);
	std::string listEntry;
	std::vector<std::string> splitList;

	while (std::getline(listStream, listEntry, separator))
	{
		splitList.push_back(listEntry);
	}
	return splitList;
}

inline void addLine(std::ostream& stream, const std::string& string) {
	for (unsigned int i = 0; i < indentationLevel; ++i) {
		stream << "	";
	}
	stream << string << "\n";
}

inline void addEnumOperator(std::ostream& cppFile, const std::string& enumName, const std::string& operatorName) {
	addLine(cppFile, "inline " + enumName + " operator" + operatorName + "(" + enumName + " one, " + enumName + " other) { ");
	++indentationLevel;
	addLine(cppFile, "return static_cast<" + enumName + ">(static_cast<unsigned int>(one) " + operatorName);
	addLine(cppFile, "static_cast<unsigned int>(other));");
	--indentationLevel;
	addLine(cppFile, "}");
}

inline void removeTags(const std::vector<std::string> extensionTags, std::string& taggedString, bool removeUnderscore = false) {
	for (auto& tag : extensionTags) {
		std::string searchValue = tag;
		if (removeUnderscore) {
			searchValue = "_" + searchValue;
		}
		if (taggedString.length() < searchValue.length()) continue;
		//Verify the structure name ends with the tag, and replace it
		size_t expectedTagOffset = taggedString.length() - searchValue.length();
		if (taggedString.find(searchValue, expectedTagOffset) == expectedTagOffset) {
			taggedString.erase(taggedString.begin() + expectedTagOffset, taggedString.end());
			break;
		}
	}
}

inline bool hasTags(const std::vector<std::string> extensionTags, const std::string& taggedString) {
	for (auto& tag : extensionTags) {
		std::string searchValue = tag;
		if (taggedString.length() < searchValue.length()) continue;
		//Verify the structure name ends with the tag, and replace it
		size_t expectedTagOffset = taggedString.length() - searchValue.length();
		if (taggedString.find(searchValue, expectedTagOffset) == expectedTagOffset) {
			return true;
		}
	}
	return false;
}

inline bool isSpecConsideredUpper(char character) {
	return isupper(character) || isdigit(character);
}

inline std::string structureNameToEnumValue(const std::string& name) {
	std::string newName;
	for (size_t i = 0; i < name.length(); ++i) {
		//Do not check the first character for being uppercase,
		//otherwise one underscore is always prepended.
		//Continuously capitalized texts (like "EXT") aren't separated by underscores.
		if (isSpecConsideredUpper(name[i]) && i && !isSpecConsideredUpper(name[i - 1])) {
			newName.push_back('_');
			newName.push_back(name[i]);
		}
		else {
			newName.push_back(toupper(name[i]));
		}
	}
	//Remove the "FlagBits" member of 
	size_t flagBitOffset = newName.find("FLAG_BITS");
	if (flagBitOffset != std::string::npos) {
		newName.replace(flagBitOffset, 10, "");
	}

	newName.push_back('_'); //In Vulkan enums, the value following the structure name is separated by another _
	return newName;
}