#include <StdAfx.h>
#include <Interpret/SmartView/NickNameCache.h>
#include <Interpret/String.h>
#include <Interpret/SmartView/PropertyStruct.h>

namespace smartview
{
	NickNameCache::NickNameCache()
	{
		m_ulMajorVersion = 0;
		m_ulMinorVersion = 0;
		m_cRowCount = 0;
		m_lpRows = nullptr;
		m_cbEI = 0;
	}

	void NickNameCache::Parse()
	{
		m_Metadata1 = m_Parser.GetBYTES(4);
		m_ulMajorVersion = m_Parser.Get<DWORD>();
		m_ulMinorVersion = m_Parser.Get<DWORD>();
		m_cRowCount = m_Parser.Get<DWORD>();

		if (m_cRowCount && m_cRowCount < _MaxEntriesEnormous)
			m_lpRows = reinterpret_cast<LPSRow>(AllocateArray(m_cRowCount, sizeof SRow));

		if (m_lpRows)
		{
			for (DWORD i = 0; i < m_cRowCount; i++)
			{
				m_lpRows[i].cValues = m_Parser.Get<DWORD>();

				if (m_lpRows[i].cValues && m_lpRows[i].cValues < _MaxEntriesSmall)
				{
					const size_t cbBytesRead = 0;
					m_lpRows[i].lpProps = NickNameBinToSPropValueArray(m_lpRows[i].cValues);
					m_Parser.Advance(cbBytesRead);
				}
			}
		}

		m_cbEI = m_Parser.Get<DWORD>();
		m_lpbEI = m_Parser.GetBYTES(m_cbEI, _MaxBytes);
		m_Metadata2 = m_Parser.GetBYTES(8);
	}

	_Check_return_ LPSPropValue NickNameCache::NickNameBinToSPropValueArray(DWORD dwPropCount)
	{
		if (!dwPropCount || dwPropCount > _MaxEntriesSmall) return nullptr;

		const auto pspvProperty = reinterpret_cast<LPSPropValue>(AllocateArray(dwPropCount, sizeof SPropValue));
		if (!pspvProperty) return nullptr;

		for (DWORD i = 0; i < dwPropCount; i++)
		{
			pspvProperty[i] = NickNameBinToSPropValue();
		}

		return pspvProperty;
	}

	_Check_return_ SPropValue NickNameCache::NickNameBinToSPropValue()
	{
		auto prop = SPropValue{};
		const auto PropType = m_Parser.Get<WORD>();
		const auto PropID = m_Parser.Get<WORD>();

		prop.ulPropTag = PROP_TAG(PropType, PropID);
		prop.dwAlignPad = 0;

		auto dwTemp = m_Parser.Get<DWORD>(); // reserved
		const auto liTemp = m_Parser.Get<LARGE_INTEGER>(); // union

		switch (PropType)
		{
		case PT_I2:
			prop.Value.i = static_cast<short int>(liTemp.LowPart);
			break;
		case PT_LONG:
			prop.Value.l = liTemp.LowPart;
			break;
		case PT_ERROR:
			prop.Value.err = liTemp.LowPart;
			break;
		case PT_R4:
			prop.Value.flt = static_cast<float>(liTemp.QuadPart);
			break;
		case PT_DOUBLE:
			prop.Value.dbl = liTemp.LowPart;
			break;
		case PT_BOOLEAN:
			prop.Value.b = liTemp.LowPart ? true : false;
			break;
		case PT_SYSTIME:
			prop.Value.ft.dwHighDateTime = liTemp.HighPart;
			prop.Value.ft.dwLowDateTime = liTemp.LowPart;
			break;
		case PT_I8:
			prop.Value.li = liTemp;
			break;
		case PT_STRING8:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.lpszA = GetStringA(dwTemp);
			break;
		case PT_UNICODE:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.lpszW = GetStringW(dwTemp / sizeof(WCHAR));
			break;
		case PT_CLSID:
			prop.Value.lpguid = reinterpret_cast<LPGUID>(GetBYTES(sizeof GUID));
			break;
		case PT_BINARY:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.bin.cb = dwTemp;
			// Note that we're not placing a restriction on how large a binary property we can parse. May need to revisit this.
			prop.Value.bin.lpb = GetBYTES(prop.Value.bin.cb);
			break;
		case PT_MV_BINARY:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.MVbin.cValues = dwTemp;
			if (prop.Value.MVbin.cValues && prop.Value.MVbin.cValues < _MaxEntriesLarge)
			{
				prop.Value.MVbin.lpbin = reinterpret_cast<LPSBinary>(AllocateArray(dwTemp, sizeof SBinary));
				if (prop.Value.MVbin.lpbin)
				{
					for (ULONG j = 0; j < prop.Value.MVbin.cValues; j++)
					{
						dwTemp = m_Parser.Get<DWORD>();
						prop.Value.MVbin.lpbin[j].cb = dwTemp;
						// Note that we're not placing a restriction on how large a multivalued binary property we can parse. May need to revisit this.
						prop.Value.MVbin.lpbin[j].lpb = GetBYTES(prop.Value.MVbin.lpbin[j].cb);
					}
				}
			}
			break;
		case PT_MV_STRING8:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.MVszA.cValues = dwTemp;
			if (prop.Value.MVszA.cValues && prop.Value.MVszA.cValues < _MaxEntriesLarge)
			{
				prop.Value.MVszA.lppszA = reinterpret_cast<LPSTR*>(AllocateArray(dwTemp, sizeof LPVOID));
				if (prop.Value.MVszA.lppszA)
				{
					for (ULONG j = 0; j < prop.Value.MVszA.cValues; j++)
					{
						prop.Value.MVszA.lppszA[j] = GetStringA();
					}
				}
			}
			break;
		case PT_MV_UNICODE:
			dwTemp = m_Parser.Get<DWORD>();
			prop.Value.MVszW.cValues = dwTemp;
			if (prop.Value.MVszW.cValues && prop.Value.MVszW.cValues < _MaxEntriesLarge)
			{
				prop.Value.MVszW.lppszW = reinterpret_cast<LPWSTR*>(AllocateArray(dwTemp, sizeof LPVOID));
				if (prop.Value.MVszW.lppszW)
				{
					for (ULONG j = 0; j < prop.Value.MVszW.cValues; j++)
					{
						prop.Value.MVszW.lppszW[j] = GetStringW();
					}
				}
			}
			break;
		default:
			break;
		}

		return prop;
	}

	_Check_return_ std::wstring NickNameCache::ToStringInternal()
	{
		auto szNickNameCache = strings::formatmessage(IDS_NICKNAMEHEADER);
		szNickNameCache += strings::BinToHexString(m_Metadata1, true);

		szNickNameCache +=
			strings::formatmessage(IDS_NICKNAMEROWCOUNT, m_ulMajorVersion, m_ulMinorVersion, m_cRowCount);

		if (m_cRowCount && m_lpRows)
		{
			for (DWORD i = 0; i < m_cRowCount; i++)
			{
				if (i > 0) szNickNameCache += L"\r\n";
				szNickNameCache += strings::formatmessage(IDS_NICKNAMEROWS, i, m_lpRows[i].cValues);

				szNickNameCache += PropsToString(m_lpRows[i].cValues, m_lpRows[i].lpProps);
			}
		}

		szNickNameCache += L"\r\n";
		szNickNameCache += strings::formatmessage(IDS_NICKNAMEEXTRAINFO);
		szNickNameCache += strings::BinToHexString(m_lpbEI, true);

		szNickNameCache += strings::formatmessage(IDS_NICKNAMEFOOTER);
		szNickNameCache += strings::BinToHexString(m_Metadata2, true);

		return szNickNameCache;
	}
}