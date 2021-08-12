# VulkanEnumClasses
A generator to generate type-safe C++ "enum class" versions of Vulkan enums.
# Installation requirements
- CMake 3.15 or higher  
- Vulkan SDK 1.2.135 or higher, alternatively a valid vk.xml file
# Installing
Prefer using the CMake GUI or IDE integrations over the command line when generating the project for easier access to the generation options. 

### Generated header location

When generation is complete, the generated header file can be found in `include/VulkanEnums.hpp` in both the source and the build directory.

# Integration in other CMake projects
VulkanEnumClasses can be integrated in other CMake projects using the `add_subdirectory` command:  
```
add_subdirectory(VulkanEnumClasses)
```
Specifying `target_link_libraries` is not needed since the library is header-only.

In source files using Vulkan enum classes, add
```cpp
#include <VulkanEnums.hpp>
```

# Generation options

To change the path of the vk.xml file, use the `XMLPATH` option of type `STRING` The path must include "vk.xml".  
Valid Example: `C:\Users\User\Documents\vk.xml`

There are multiple options to customize the code style of the header.

| Name | Default value | Type | Description |
| ---- | ------------  | ---- | ----------- |
| `NAMESPACE` | | `STRING` | An optional namespace in which all enums will be defined. Empty means no namespace. There must not be any whitespace in the string.
| `AUTO_INCLUDE_EXTENSIONS` | `TRUE` | `BOOL` | When enabled, all enums defined in vk.xml except explicitly excluded ones will be generated. When disabled, only core enums and explicitly enabled extensions will be generated. |
| `EXTENSION_EXCLUDES` | | `STRING` | Names of extensions whose enums to ignore, spearated by \';\'. Has no effect if AUTO_INCLUDE_EXTENSIONS is FALSE. There must not be any whitespace in the string. Disabling an extension will automatically disable all extensions that depend on it. |
| `EXTENSION_INCLUDES` | | `STRING` | Names of extensions whose enums to include, spearated by \';\'. Has no effect if AUTO_INCLUDE_EXTENSIONS is TRUE. There must not be any whitespace in the string. Enabling an extension will automatically enable all extensions it depends on. |
| `REPLACE_NAMES` | `TRUE` | `BOOL` | Toggles whether the enum names will be changed. If this value is false, all options modifying enum names have no effect. |
| `NAME_PREFIX_REPLACEMENT` | | `STRING` | A string that prefixes all enum names instead of \"Vk\". There must not be any whitespace in the string. |
| `NAME_REMOVE_POSTFIX` | `FALSE` | `BOOL` | Toggles whether the postfixes on extension enum names (e.g. EXT, KHR) will be kept or removed. |
| `REPLACE_VALUES` | `TRUE` | `BOOL` | Toggles whether to replae enum names or keep them as in vk.xml. |
| `VALUE_PREFIX_REPLACEMENT` | | `STRING` | The prefix applied to all values instead of \"VK_\". There must not be any whitespace in the string. |
| `VALUE_NUMBER_PREFIX` | `_` | `STRING` | The prefix applied to all values that have a leading digit (which is illegal). There must not be any whitespace in the string. |
| `VALUE_REMOVE_UNDERSCORES` | `TRUE` | `BOOL` | Toggles whether to remove underscores from structure names. |
| `VALUE_REMOVE_STRUCTURE_NAMES` | `TRUE` | `BOOL` | If FALSE, the name of the enum will be removed from all values of the enum. |
| `VALUE_TOLOWER` | `TRUE` | `BOOL` | If TRUE, enum values will be converted to lowercase. |
| `VALUE_CAPITALIZE_START` | `TRUE` | `BOOL` | If TRUE, the beginning of each word in the value is capitalized (ENUM_VALUE becomes EnumValue) |
| `VALUE_REMOVE_POSTFIX` | `TRUE` | `BOOL` | Toggles whether the postfixes on extension enum values (e.g. EXT, KHR) will be kept or removed. |
| `VALUE_REMOVE_POSTFIX_NONPOSTFIX_TYPE` | `FALSE` | `BOOL` | Toggles whether the postfixes on extension enum values (e.g. EXT, KHR) of structures that do not have a postfix will be kept or removed. |
