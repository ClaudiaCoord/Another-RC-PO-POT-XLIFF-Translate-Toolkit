
#include <forward_list>

namespace LANG {

	class BASEID {
	public:
		std::forward_list<std::wstring_view> short_list{};
		std::wstring_view long_name;
		std::wstring_view lang_string;

		BASEID(const std::wstring_view l, const std::wstring_view ln)
			: lang_string(std::wstring_view(l)), long_name(std::wstring_view(ln)) {
		}
		BASEID(const std::wstring_view l, const std::wstring_view ln, const std::wstring_view sn)
			: lang_string(std::wstring_view(l)), long_name(std::wstring_view(ln)) {
			short_list.push_front(std::wstring_view(sn));
		}
		BASEID(const std::wstring_view l, const std::wstring_view ln, const std::wstring_view sn1, const std::wstring_view sn2)
			: lang_string(std::wstring_view(l)), long_name(std::wstring_view(ln)) {
			short_list.push_front(std::wstring_view(sn1));
			short_list.push_front(std::wstring_view(sn2));
		}
	};

	class Langid {
	private:
		std::vector<BASEID> list_{};

		static constexpr std::wstring_view DEFAULT_LANG_ = L"LANG_SYSTEM_DEFAULT, SUBLANG_DEFAULT"sv;

	public:

		Langid() {
			#include "LangID-internal.h"
		}

		void addLong(const std::wstring_view l, const std::wstring_view ln) {
			list_.push_back(BASEID(l, ln));
		}
		void addLong(const std::wstring_view l, const std::wstring_view ln, const std::wstring_view sn) {
			list_.push_back(BASEID(l, ln, sn, L""sv));
		}
		void addLong(const std::wstring_view l, const std::wstring_view ln, const std::wstring_view sn1, const std::wstring_view sn2) {
			list_.push_back(BASEID(l, ln, sn1, sn2));
		}
		void addShort(const std::wstring_view l, const std::wstring_view sn) {
			list_.push_back(BASEID(l, L""sv, sn));
		}
		void addShort(const std::wstring_view l, const std::wstring_view sn1, const std::wstring_view sn2) {
			list_.push_back(BASEID(l, L""sv, sn1, sn2));
		}

		const std::wstring find(const std::wstring& s) {
			try {
				bool char2_ = (s.length() == 2);
				std::wstring p = std::wstring(s);
				if (char2_)
					std::transform(p.begin(), p.end(), p.begin(), ::toupper);
				else {
					size_t pos = p.find_first_of(separators::dash);
					if (pos != std::wstring::npos)
						p = p.replace(pos, 1, L"_");
				}

				auto it = std::find_if(list_.begin(), list_.end(), [char2_, p](BASEID& a) {
						if (char2_) {
							for (auto& n : a.short_list)
								if (n._Equal(p)) return true;
							return false;
						}
						if (a.long_name._Equal(p)) return true;
						return false;
					}
				);
				if (it != list_.end())
					return (static_cast<BASEID*>(&*it))->lang_string.data();
			}
			catch (...) {
				cw.print_exception(std::current_exception(), __FUNCTIONW__);
			}
			return DEFAULT_LANG_.data();
		}
	};

	class LangUtil {
	private:
		static constexpr std::wstring_view DEFAULT_SUBLANG_ = L"SUBLANG_DEFAULT"sv;

	public:
		static std::wstring check_sublang(const std::wstring& s) {
			try{
				if (s.empty()) return std::wstring();

				size_t pos = s.find_first_of(separators::comma);
				if (pos != std::wstring::npos) return std::wstring(s);
				return (std::wstringstream() << s.c_str() << L", " << DEFAULT_SUBLANG_.data()).str();
			}
			catch (...) {
				cw.print_exception(std::current_exception(), __FUNCTIONW__);
			}
			return std::wstring();
		}
	};
}
