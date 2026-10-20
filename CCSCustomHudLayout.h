#pragma once
#include "CBaseEntity.h"
#include "module.h"
#include <utlvector.h>
#include <utlstring.h>

enum EHudPanelClassStatus_t : int
{
	k_eHudPanelClassStatus_Undefined = -1,
	k_eHudPanelClassStatus_DoesNotHaveClass = 0,
	k_eHudPanelClassStatus_HasClass = 1,
};

struct HUDPanelHasClass_t
{
	uint16 m_nPanelIdIndex;
	uint16 m_nClassNameIndex;
	EHudPanelClassStatus_t m_eClassStatus;

	bool operator==(const HUDPanelHasClass_t& other) const
	{
		return m_nPanelIdIndex == other.m_nPanelIdIndex && m_nClassNameIndex == other.m_nClassNameIndex;
	}
};

struct HUDPanelDialogVariableString_t
{
	void* m_pVTable;
	uint16 m_nPanelIdIndex;
	uint16 m_nDialogVariableIndex;
	CUtlString m_sValue;
	bool m_bIsSet;

	bool operator==(const HUDPanelDialogVariableString_t& other) const
	{
		return m_nPanelIdIndex == other.m_nPanelIdIndex && m_nDialogVariableIndex == other.m_nDialogVariableIndex;
	}
};

class CCSCustomHudLayoutState
{
public:
	DECLARE_SCHEMA_CLASS_INLINE(CCSCustomHudLayoutState)

	SCHEMA_FIELD(int, m_playerSlot)
	SCHEMA_FIELD(bool, m_bInputCaptureEnabled)
	SCHEMA_FIELD_POINTER(CUtlVector<HUDPanelHasClass_t>, m_vecHasClasses)
	SCHEMA_FIELD_POINTER(CUtlVector<HUDPanelDialogVariableString_t>, m_vecDialogVariableStrings)
};

class CCSCustomHudLayout : public CBaseEntity
{
public:
	DECLARE_SCHEMA_CLASS(CCSCustomHudLayout)

	SCHEMA_FIELD(CUtlSymbolLarge, m_strLayout)
	SCHEMA_FIELD_POINTER(void, m_vecPlayerLayoutStates)
	SCHEMA_FIELD_POINTER(CCSCustomHudLayoutState, m_globalLayoutState)
	SCHEMA_FIELD_POINTER(CUtlVector<CUtlString>, m_vecPanelIds)
	SCHEMA_FIELD_POINTER(CUtlVector<CUtlString>, m_vecClassNames)
	SCHEMA_FIELD_POINTER(CUtlVector<CUtlString>, m_vecDialogVariableNames)

	void SetHasClass(int iSlot, const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(pState) ApplyHasClass(pState, szPanelId, szClassName, bHasClass);
	}

	void SetGlobalHasClass(const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		ApplyHasClass(m_globalLayoutState(), szPanelId, szClassName, bHasClass);
	}

	void SetDialogVariableString(int iSlot, const char* szPanelId, const char* szVariableName, const char* szValue)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(pState) ApplyDialogVariable(pState, szPanelId, szVariableName, szValue);
	}

	void SetGlobalDialogVariableString(const char* szPanelId, const char* szVariableName, const char* szValue)
	{
		ApplyDialogVariable(m_globalLayoutState(), szPanelId, szVariableName, szValue);
	}

	const char* GetDialogVariableString(int iSlot, const char* szPanelId, const char* szVariableName)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(!pState) return "";

		int iPanel = m_vecPanelIds()->Find(CUtlString(szPanelId));
		int iVariable = m_vecDialogVariableNames()->Find(CUtlString(szVariableName));
		if(iPanel == -1 || iVariable == -1) return "";

		CUtlVector<HUDPanelDialogVariableString_t>* pVariables = pState->m_vecDialogVariableStrings();
		for(int i = 0; i < pVariables->Count(); i++)
		{
			HUDPanelDialogVariableString_t& variable = pVariables->Element(i);
			if(variable.m_nPanelIdIndex == iPanel && variable.m_nDialogVariableIndex == iVariable)
				return variable.m_sValue.Get();
		}
		return "";
	}

	void SetInputCaptureEnabled(int iSlot, bool bEnable)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		if(!pState) return;
		pState->m_bInputCaptureEnabled() = bEnable;
		m_vecPlayerLayoutStates.NetworkStateChanged();
	}

	void SetGlobalInputCaptureEnabled(bool bEnable)
	{
		m_globalLayoutState()->m_bInputCaptureEnabled() = bEnable;
		m_globalLayoutState.NetworkStateChanged();
	}

	bool GetInputCaptureEnabled(int iSlot)
	{
		CCSCustomHudLayoutState* pState = GetPlayerState(iSlot);
		return pState && pState->m_bInputCaptureEnabled();
	}

private:
	CCSCustomHudLayoutState* GetPlayerState(int iSlot)
	{
		static const SchemaKey key = schema::GetOffset("CCSCustomHudLayout", hash_32_fnv1a_const("CCSCustomHudLayout"), "m_vecPlayerLayoutStates", hash_32_fnv1a_const("m_vecPlayerLayoutStates"));
		if(!key.manipulator || iSlot < 0) return nullptr;

		void* pStates = m_vecPlayerLayoutStates();
		int iCount = (int)(intptr_t)key.manipulator(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_COUNT, pStates, 0, 0);
		if(iSlot >= iCount) return nullptr;

		return (CCSCustomHudLayoutState*)key.manipulator(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT, pStates, iSlot, 0);
	}

	int FindOrAddName(CUtlVector<CUtlString>* pNames, const char* szName)
	{
		int iIndex = pNames->Find(CUtlString(szName));
		if(iIndex == -1) iIndex = pNames->AddToTail(CUtlString(szName));
		return iIndex;
	}

	void ApplyHasClass(CCSCustomHudLayoutState* pState, const char* szPanelId, const char* szClassName, bool bHasClass)
	{
		HUDPanelHasClass_t hasClass;
		hasClass.m_nPanelIdIndex = FindOrAddName(m_vecPanelIds(), szPanelId);
		hasClass.m_nClassNameIndex = FindOrAddName(m_vecClassNames(), szClassName);
		hasClass.m_eClassStatus = bHasClass ? k_eHudPanelClassStatus_HasClass : k_eHudPanelClassStatus_DoesNotHaveClass;

		CUtlVector<HUDPanelHasClass_t>* pClasses = pState->m_vecHasClasses();
		int iIndex = pClasses->Find(hasClass);
		if(iIndex == -1) pClasses->AddToTail(hasClass);
		else pClasses->Element(iIndex).m_eClassStatus = hasClass.m_eClassStatus;

		NotifyChanged(pState);
	}

	void ApplyDialogVariable(CCSCustomHudLayoutState* pState, const char* szPanelId, const char* szVariableName, const char* szValue)
	{
		static void* pVTable = DynLibUtils::CModule(g_pSource2Server).GetVirtualTableByName("HUDPanelDialogVariableString_t");

		HUDPanelDialogVariableString_t variable;
		variable.m_pVTable = pVTable;
		variable.m_nPanelIdIndex = FindOrAddName(m_vecPanelIds(), szPanelId);
		variable.m_nDialogVariableIndex = FindOrAddName(m_vecDialogVariableNames(), szVariableName);
		variable.m_sValue = szValue;
		variable.m_bIsSet = true;

		CUtlVector<HUDPanelDialogVariableString_t>* pVariables = pState->m_vecDialogVariableStrings();
		int iIndex = pVariables->Find(variable);
		if(iIndex == -1) pVariables->AddToTail(variable);
		else pVariables->Element(iIndex).m_sValue = szValue;

		NotifyChanged(pState);
	}

	void NotifyChanged(CCSCustomHudLayoutState* pState)
	{
		m_vecPanelIds.NetworkStateChanged();
		m_vecClassNames.NetworkStateChanged();
		m_vecDialogVariableNames.NetworkStateChanged();
		if(pState == m_globalLayoutState()) m_globalLayoutState.NetworkStateChanged();
		else m_vecPlayerLayoutStates.NetworkStateChanged();
	}
};
