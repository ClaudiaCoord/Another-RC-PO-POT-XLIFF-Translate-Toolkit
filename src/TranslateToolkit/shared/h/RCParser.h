/*
	Another RC/PO/POT/XLIFF Translate Toolkit.
	(c) CC 2023, MIT
	See README.md for more details.
*/

#pragma once

using namespace std::string_view_literals;

namespace PARSERS {

    template<class T1>
    class RCParser {
    private:

        static constexpr std::wstring_view Names[] = {
            L"-"sv,
            L"ICON"sv,
            L"EDITTEXT"sv,
            L"COMBOBOX"sv,
            L"BEGIN"sv,
            L"END"sv,
            L"STRINGTABLE"sv,
            L"STYLE "sv,
            L"FONT "sv,
            L"MENU "sv
        };

        T1& config_;
        LANGLIST dictionary_{};
        EXCLUDELIST exclude_list_{};

    public:

        RCParser(T1& c) : config_(c) {
            utils::build_exclude_list(exclude_list_, config_->path_exclude);
        }

        std::wstring parse(std::wstring& line) {
            try {
                if (line.empty())
                    return std::wstring();

                #if defined (_DEBUG_V)
                {
                    size_t n0 = line.find(L"POPUP");
                    if (n0 != std::wstring::npos) {
                        ::OutputDebugStringW(line.c_str());
                    }
                }
                #endif  

                do {
                    const auto c = line.at(0);
                    if (c == L'L') {
                        if (line.starts_with(L"LANGUAGE ") && !config_->lang_id.empty())
                            return (std::wstringstream() << L"LANGUAGE " << config_->lang_id.c_str() << L"\n").str();
                    }

                    {
                        std::wstring w(line);
                        size_t n0 = w.find_first_not_of(L" \r\n\t");
                        if (n0 != std::wstring::npos)
                            w = w.substr(n0);
                        for (auto& a : Names)
                            if (w.starts_with(a)) return std::wstring(line);
                    }

                    size_t n1 = line.find_first_of(separators::text);
                    if (n1 == std::wstring::npos) break;
                    size_t n2 = line.find_first_of(separators::text, (n1 + 1));
                    if (n2 == std::wstring::npos) break;

                    std::wstring line_ = line.substr((n1 + 1), (n2 - n1 - 1));
                    if (line_.empty() || (line_.at(0) == L'"')) break;

                    for (auto& a : exclude_list_)
                        if (line_._Equal(a)) return std::wstring(line);

                    auto it = std::find_if(dictionary_.begin(), dictionary_.end(), [line_](LANGP& a) {
                        return a.first._Equal(line_);
                        });
                    if (it == dictionary_.end()) break;

                    LANGP* li = static_cast<LANGP*>(&*it);
                    return line.replace((n1 + 1), line_.length(), li->second);

                } while (0);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring(line);
        }

        const bool empty_dictionary() {
            return dictionary_.empty();
        }
        void add_dictionary(std::wstring& orig, std::wstring& trans) {

            if (config_->is_normalize && (trans.length() > 1))
                dictionary_.push_back(std::make_pair(orig, utils::po_normalize(trans)));
            else dictionary_.push_back(std::make_pair(orig, trans));
        }
    };
}
