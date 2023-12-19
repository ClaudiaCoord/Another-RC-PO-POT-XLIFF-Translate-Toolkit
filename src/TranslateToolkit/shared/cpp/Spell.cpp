/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#define _NO_CONFIGINFO 1
#include "..\h\global.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlstr.h>
#include <objidl.h>
#include <spellcheck.h>

#include "..\h\Spell.h"

namespace SPELL {

	Corrector::~Corrector() {
		dispose_();
	}
	void Corrector::dispose_() {
		checker_.Release();
		factory_.Release();
	}
	std::wstring Corrector::replace_(std::wstring& s, const std::wstring& w, const wchar_t* r) {
		std::wstring txt(s);
		if (!r || w.empty()) return txt;

		size_t pos{ 0 };
		while (pos != std::wstring::npos) {
			pos = txt.find(w, pos);
			if (pos != std::wstring::npos)
				txt = txt.replace(pos++, w.length(), r);
		}
		return txt;
	}
	void Corrector::select_(const std::wstring& lang) {

		if (!factory_) throw std::runtime_error("checker not initialize");

		BOOL b{ FALSE };

		if (factory_->IsSupported(lang.c_str(), &b) != S_OK)
			throw std::runtime_error("check support error");

		if (!b) throw std::runtime_error("language not support!");

		if (factory_->CreateSpellChecker(lang.c_str(), (ISpellChecker**)&checker_) != S_OK)
			throw std::runtime_error("create error");
	}
	const bool Corrector::check_(const std::wstring& s, int32_t count, bool is_auto) {
		if (!factory_ || !checker_ || s.empty()) return false;

		try {
			suggestions_list_.clear();
			replacements_list_.clear();
			bool is_correct_{ false };

			std::wstring rs = std::wstring(s);

			CComPtr<IEnumSpellingError> err_ptr{};
			if (checker_->Check((LPCWSTR)rs.c_str(), (IEnumSpellingError**)&err_ptr) != S_OK) {
				err_ptr.Release();
				return false;
			}
			for (;;) {

				std::wstring word{};
				///
				{
					CComPtr<ISpellingError> e_ptr_{};
					if (err_ptr->Next((ISpellingError**)&e_ptr_) != S_OK) {
						e_ptr_.Release();
						break;
					}
					if (ULONG i, l; (e_ptr_->get_StartIndex(&i) == S_OK) && (e_ptr_->get_Length(&l) == S_OK)) {

						word = s.substr(i, l);
						if (!word.empty()) {

							CORRECTIVE_ACTION action{};
							if (e_ptr_->get_CorrectiveAction(&action) == S_OK) {

								if (action != CORRECTIVE_ACTION::CORRECTIVE_ACTION_GET_SUGGESTIONS) {
									e_ptr_.Release();
									continue;
								}

								if (!is_auto) {
									wchar_t* xword{ nullptr };
									if (e_ptr_->get_Replacement(&xword) == S_OK) {
										replacements_list_.push_back(std::make_pair(word, std::wstring(xword)));
										::CoTaskMemFree(xword);
									}
								}
							}
						}
					}
					e_ptr_.Release();
				}
				///
				if (!word.empty()) {

					CComPtr<IEnumString> s_ptr_{};
					if (checker_->Suggest(word.c_str(), (IEnumString**)&s_ptr_) == S_OK) {

						for (uint32_t i = 0, x = ((count < 0) ? 50 : count); i < x; i++) {

							wchar_t* xword{};
							if (s_ptr_->Next(1, &xword, nullptr) != S_OK) {
								s_ptr_.Release();
								break;
							}

							is_correct_ = true;

							if (is_auto) rs = replace_(rs, word, xword);
							else
								suggestions_list_.push_back(
									std::make_pair(
										xword, replace_(rs, word, xword)
									)
								);

							::CoTaskMemFree(xword);
							if (is_auto) break;
						}
					}
					s_ptr_.Release();
				}
			}

			err_ptr.Release();
			if (is_auto && is_correct_ && !rs.empty()) replacements_list_.push_back(std::make_pair(rs, L""));
			return is_auto ? !replacements_list_.empty() : (!replacements_list_.empty() || !suggestions_list_.empty());
		}
		catch (...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}
		return false;
	}

	FOUNDLIST& Corrector::GetReplacements() {
		return std::ref(replacements_list_);
	}
	FOUNDLIST& Corrector::GetSuggestions() {
		return std::ref(suggestions_list_);
	}

	const bool Corrector::Init(const std::wstring lang) {
		bool is_init_{ false };
		try {
			dispose_();

			if (::CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK)
				throw std::runtime_error("CoInitialize error");

			if (::CoCreateInstance(__uuidof(SpellCheckerFactory),
				nullptr, CLSCTX_INPROC_SERVER,
				__uuidof(factory_), (void**)&factory_) != S_OK)
				throw std::runtime_error("CoCreate error");

			if (!lang.empty())
				select_(lang);

			is_init_ = true;
		} catch (...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}

		if (!is_init_) {
			checker_.Release();
			factory_.Release();
		}
		return is_init_;
	}
	std::vector<std::wstring> Corrector::List() {
		CComPtr<IEnumString> s_ptr{};
		std::vector<std::wstring> v{};

		try {
			if (!factory_) return v;

			if (factory_->get_SupportedLanguages((IEnumString**)&s_ptr) != S_OK)
				throw std::runtime_error("language list error");

			HRESULT r{ S_OK };
			while (S_OK == r) {
				LPOLESTR s{};
				r = s_ptr->Next(1, &s, nullptr);
				if (r != S_OK) break;
				v.push_back(s);
				::CoTaskMemFree(s);
			}
		} catch(...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}
		s_ptr.Release();
		return v;
	}
	const bool Corrector::Select(const std::wstring lang) {
		try {
			if (factory_ || lang.empty()) return false;
			select_(lang);
			return true;

		} catch (...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}
		return false;
	}

	const bool Corrector::AddStopWord(const std::wstring& s) {
		try {
			if (!factory_ || !checker_ || s.empty()) return false;
			return checker_->Add(s.c_str()) == S_OK;
		}
		catch (...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}
		return false;
	}
	const bool Corrector::AddStopWords(STOPLIST& list) {
		try {
			if (!factory_ || !checker_ || list.empty()) return false;
			for (auto& a : list)
				if (checker_->Add(a.c_str()) != S_OK) return false;
			return true;
		}
		catch (...) {
			cw.print_exception(std::current_exception(), __FUNCTIONW__);
		}
		return false;
	}

	const bool Corrector::AutoCorrect(const std::wstring& s) {
		return check_(s, 1, true);
	}
	const bool Corrector::Check(const std::wstring& s, int32_t count) {
		return check_(s, count, false);
	}
}