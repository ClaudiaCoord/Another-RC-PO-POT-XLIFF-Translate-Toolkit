/*
	Another RC/PO/POT/XLIFF Translate Toolkit.
	(c) CC 2023, MIT
	See README.md for more details.
*/

#pragma once

typedef std::vector<std::pair<std::wstring, std::wstring>> FOUNDLIST;
typedef std::vector<std::wstring> STOPLIST;

namespace SPELL {
	class FLAG_EXPORT Corrector {
	private:

		CComPtr<ISpellCheckerFactory> factory_{};
		CComPtr<ISpellChecker> checker_{};

		FOUNDLIST replacements_list_{};
		FOUNDLIST suggestions_list_{};

		void dispose_();
		void select_(const std::wstring&);
		const bool check_(const std::wstring&, int32_t = -1, bool = false);
		std::wstring replace_(std::wstring&, const std::wstring&, const wchar_t*);

	public:

		Corrector() = default;
		~Corrector();

		const bool Init(const std::wstring = L"");
		const bool Select(const std::wstring);
		const bool Check(const std::wstring&, int32_t);
		const bool AutoCorrect(const std::wstring&);
		std::vector<std::wstring> List();

		const bool AddStopWord(const std::wstring&);
		const bool AddStopWords(STOPLIST&);

		FOUNDLIST& GetReplacements();
		FOUNDLIST& GetSuggestions();
	};
}

