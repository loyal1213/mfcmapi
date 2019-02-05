#pragma once

// MrMAPI command line
namespace cli
{
	struct COMMANDLINE_SWITCH
	{
		int iSwitch;
		LPCWSTR szSwitch;
	};

	struct OptParser
	{
		int clSwitch{};
		int mode{};
		int minArgs{};
		int maxArgs{};
		int options{};
	};

	OptParser GetParser(int clSwitch, const std::vector<OptParser>& parsers);

	// Checks if szArg is an option, and if it is, returns which option it is
	// We return the first match, so switches should be ordered appropriately
	// The first switch should be our "no match" switch
	int ParseArgument(const std::wstring& szArg, const std::vector<COMMANDLINE_SWITCH>& switches);

	// If the mode isn't set (is 0), then we can set it to any mode
	// If the mode IS set (non 0), then we can only set it to the same mode
	// IE trying to change the mode from anything but unset will fail
	template <typename M> bool bSetMode(_In_ M& pMode, _In_ M targetMode)
	{
		if (0 == pMode || targetMode == pMode)
		{
			pMode = targetMode;
			return true;
		}

		return false;
	}

	std::vector<std::wstring> GetCommandLine(_In_ int argc, _In_count_(argc) const char* const argv[]);
} // namespace cli