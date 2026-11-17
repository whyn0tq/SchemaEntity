// DynLibUtils
// Copyright (C) 2023 komashchenko (Phoenix)
// https://github.com/komashchenko/DynLibUtils

#include "module.h"
#include "memaddr.h"
#include "pattern_scan.h"

using namespace DynLibUtils;

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : szModuleName (without extension .dll/.so)
//-----------------------------------------------------------------------------
CModule::CModule(const std::string_view szModuleName) : m_pModuleHandle(nullptr)
{
	InitFromName(szModuleName);
}

//-----------------------------------------------------------------------------
// Purpose: constructor
// Input  : pModuleMemory
//-----------------------------------------------------------------------------
CModule::CModule(const CMemory pModuleMemory) : m_pModuleHandle(nullptr)
{
	InitFromMemory(pModuleMemory);
}

//-----------------------------------------------------------------------------
// Purpose: Converts a string pattern with wildcards to an array of bytes and mask
// Input  : svInput
// Output : std::pair<std::vector<uint8_t>, std::string>
//-----------------------------------------------------------------------------
std::pair<std::vector<uint8_t>, std::string> CModule::PatternToMaskedBytes(const std::string_view svInput)
{
	return ParsePattern(svInput);
}

//-----------------------------------------------------------------------------
// Purpose: Finds an array of bytes in process memory
// Input  : *pPattern
//          szMask
//          pStartAddress
//          *pModuleSection
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPattern(const CMemory pPattern, const std::string_view szMask, const CMemory pStartAddress, const ModuleSections_t* pModuleSection) const
{
	const auto* section = pModuleSection ? pModuleSection : &m_ExecutableCode;
	if (!section->IsSectionValid())
		return CMemory();

	const uintptr_t base = section->m_pSectionBase;
	const uintptr_t start = pStartAddress ? static_cast<uintptr_t>(pStartAddress) : base;
	if (start < base || start - base >= section->m_nSectionSize)
		return CMemory();

	const auto* data = reinterpret_cast<const uint8_t*>(start);
	const size_t size = section->m_nSectionSize - (start - base);
	return ScanPattern(data, size, pPattern.RCast<const uint8_t*>(), szMask);
}

//-----------------------------------------------------------------------------
// Purpose: Finds a string pattern in process memory
// Input  : svPattern
//          pStartAddress
//          *pModuleSection
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CModule::FindPattern(const std::string_view svPattern, const CMemory pStartAddress, const ModuleSections_t* pModuleSection) const
{
	const std::pair patternInfo = PatternToMaskedBytes(svPattern);
	return FindPattern(patternInfo.first.data(), patternInfo.second, pStartAddress, pModuleSection);
}

//-----------------------------------------------------------------------------
// Purpose: Gets a module section by name (example: '.rdata', '.text')
// Input  : svModuleName
// Output : ModuleSections_t
//-----------------------------------------------------------------------------
CModule::ModuleSections_t CModule::GetSectionByName(const std::string_view svSectionName) const
{
	for (const ModuleSections_t& section : m_vModuleSections)
	{
		if (section.m_svSectionName == svSectionName)
			return section;
	}

	return ModuleSections_t();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the module handle
//-----------------------------------------------------------------------------
void* CModule::GetModuleHandle() const noexcept
{
	return m_pModuleHandle;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the module path
//-----------------------------------------------------------------------------
std::string_view CModule::GetModulePath() const
{
	return m_sModulePath;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the module name
//-----------------------------------------------------------------------------
std::string_view CModule::GetModuleName() const
{
	std::string_view svModulePath(m_sModulePath);
	return svModulePath.substr(svModulePath.find_last_of("/\\") + 1);
}

#ifndef DYNLIBUTILS_SEPARATE_SOURCE_FILES
	#if defined _WIN32 && _M_X64
		#include "module_windows.cpp"
	#elif defined __linux__ && __x86_64__
		#include "module_linux.cpp"
	#else
		#error "Unsupported platform"
	#endif
#endif
