/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    po2spell Spelling Checker PO/POT LANGUAGE files

    See README.md for more details.
*/

#include "global.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS
#include <atlbase.h>
#include <atlstr.h>
#include <objidl.h>
#include <spellcheck.h>

#include "Spell.h"

    class config : public info::configinfo {
    public:

        bool is_auto{ false };
        bool is_reverse{ false };
        int32_t sug_count{ -1 };
        std::wstring lang_id{};

        std::filesystem::path path_po{};
        std::filesystem::path path_out{};
        std::filesystem::path path_spell{};
        std::filesystem::path path_exclude{};

        config(argparse::ArgumentParser& args) {

            is_auto = args.exists(L"a");
            is_reverse = args.exists(L"r");

            if (args.exists(L"l")) lang_id = args.get<std::wstring>(L"l");
            else return;

            if (args.exists(L"p")) path_po = std::filesystem::path(args.get<std::wstring>(L"p"));
            else return;
            if (!std::filesystem::exists(path_po)) return;

            if (args.exists(L"o")) path_out = std::filesystem::path(args.get<std::wstring>(L"o"));
            else if (is_auto) {
                path_out = std::filesystem::path(path_po);
                path_out.replace_extension(L".spell.po");
            }

            if (args.exists(L"e"))
                path_exclude = std::filesystem::path(args.get<std::wstring>(L"e"));
            else {
                wchar_t cpath[MAX_PATH + 1]{};
                if (::GetModuleFileNameW(nullptr, cpath, MAX_PATH) != 0) {
                    path_exclude = std::filesystem::path(cpath);
                    std::wstringstream ss{};
                    path_exclude.replace_extension(
                        (std::wstringstream() << L"." << lang_id.c_str() << L".exclude").str()
                    );
                }
            }
            if (!std::filesystem::exists(path_exclude)) path_exclude = std::filesystem::path();

            if (args.exists(L"c")) sug_count = args.get<int32_t>(L"c");

            path_spell = std::filesystem::path(path_po);
            path_spell.replace_extension(L".spell");
        }

        const bool empty() {
            return lang_id.empty() || path_po.empty() || path_spell.empty();
        }
    };

    class parser {
    private:

        CONFIG& config_;
        EXCLUDELIST exclude_list_{};
        std::unique_ptr<SPELL::Corrector> sc_{};

    public:

        parser(CONFIG& c) : config_(c) {
            sc_ = std::make_unique<SPELL::Corrector>();
            try {
                utils::build_exclude_list(exclude_list_, config_->path_exclude);
                sc_->Init(config_->lang_id);
                if (!exclude_list_.empty())
                    sc_->AddStopWords(exclude_list_);
                        
            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
        }
        ~parser() {
            sc_.reset();
        }

        std::pair<std::wstring, std::wstring> parse(std::wstring& line) {
            try {
                if (line.empty())
                    return std::make_pair(L"",L"");

                do {
                    size_t n1 = line.find_first_of(separators::text);
                    if (n1 == std::wstring::npos) break;
                    size_t n2 = line.find_first_of(separators::text, (n1 + 1));
                    if (n2 == std::wstring::npos) break;

                    std::wstring line_ = line.substr((n1 + 1), (n2 - n1 - 1));
                    if (line_.empty() || (line_.at(0) == L'"')) break;

                    if (config_->is_auto && sc_->AutoCorrect(line_)) {
                        auto& v = sc_->GetReplacements();
                        if (v.empty()) break;
                        return std::make_pair(line_, line.replace(n1 + 1, (n2 - n1 - 2), v[0].first));
                    }
                    if (!config_->is_auto && sc_->Check(line_, config_->sug_count)) {
                        std::wstringstream ss{};
                        ss << L"\n\t[Bad word]:";
                        for (auto& a : sc_->GetReplacements()) {
                            ss << L"\n\t\t" << a.first;
                            if (!a.second.empty())
                                ss << L"\t | " << a.second;
                        }
                        ss << L"\n\t[Suggestions word]:";
                        for (auto& a : sc_->GetSuggestions())
                            ss << L"\n\t\t" << a.first << L"\t | " << a.second;
                        ss << L"\n\n";

                        return std::make_pair(line_, ss.str());
                    }

                } while (0);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::make_pair(L"", L"");
        }
    };

    class writer {
    private:

        enum class TypeField : int {
            NONE = 0,
            MSGID,
            MSGSTR,
        };

        std::wifstream stream_in_{};
        std::wofstream stream_out_{};
        std::wofstream stream_spell_{};
        parser parser_;
        CONFIG& config_;

        bool is_spell_write{ false };

    public:
        writer(CONFIG& c) : config_(c), parser_(parser(c)) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                stream_in_.open(config_->path_po.wstring(), std::ios::in | std::ios::binary);
                if (!stream_in_.is_open())
                    throw std::runtime_error("error open PO file");

                if (config_->is_auto) {
                    stream_out_.open(config_->path_out.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                    if (!stream_out_.is_open()) return false;
                    stream_out_.imbue(std::locale(".utf-8"));
                }

                stream_spell_.open(config_->path_spell.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_spell_.is_open()) return false;

                stream_in_.imbue(std::locale(".utf-8"));
                stream_spell_.imbue(std::locale(".utf-8"));
                {
                    std::wstring line{};
                    while (std::getline(stream_in_, line)) {
                        

                        TypeField tf{ TypeField::NONE };
                             if (line.empty() || (line.at(0) != L'm')) tf = TypeField::NONE;
                        else if (!config_->is_reverse && line.starts_with(L"msgid")) tf = TypeField::MSGID;
                        else if (config_->is_reverse && line.starts_with(L"msgstr")) tf = TypeField::MSGSTR;

                        switch (tf) {
                            using enum TypeField;
                            case MSGID:
                            case MSGSTR: {
                                std::pair<std::wstring, std::wstring> p = parser_.parse(line);
                                if (p.first.empty()) {
                                    if (config_->is_auto) write(line, true);
                                    continue;
                                }
                                if (config_->is_auto) {
                                    write(p.second, true);
                                    write_replace(p);
                                }
                                else write_suggestion(p);
                                break;
                            }
                            case NONE:
                            default: {
                                if (config_->is_auto) write(line, true);
                                break;
                            }
                        }
                    }
                }
                stream_in_.close();
                stream_spell_.flush();
                stream_spell_.close();

                if (!is_spell_write)
                    std::filesystem::remove(config_->path_spell);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return false;
        }
        void write(const std::wstring s, bool crlf = false) {
            stream_out_ << s;
            if (crlf) stream_out_ << L"\n";
        }
        void write_(const std::wstringstream s) {
            stream_out_ << s.str();
        }
        void write_replace(std::pair<std::wstring, std::wstring>& p) {
            stream_spell_ << L"\nOriginal: [" << p.first << L"]\nReplaced: [" << p.second << L"]\n";
            is_spell_write = true;
        }
        void write_suggestion(std::pair<std::wstring, std::wstring>& p) {
            stream_spell_ << L"\n" << p.first << p.second;
            is_spell_write = true;
        }
        //
        void close() {
            if (stream_in_.is_open())
                stream_in_.close();
            if (stream_spell_.is_open()) {
                stream_spell_.flush();
                stream_spell_.close();
            }
            if (stream_out_.is_open()) {
                stream_out_.flush();
                stream_out_.close();
            }
        }
    };

void args_using() {
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -s\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -l en-US -c 10\n\t\t-p x:\\path\\to\\file\\resource.po\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -l en-US -a\n\t\t-p x:\\path\\to\\file\\resource.po\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {
        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-s", L"--list" })
                .description(L"Print list of supported languages");

            args.add_argument()
                .names({ L"-l", L"--lang" })
                .description(L"Select a test language from the supported languages, Microsoft ID: en-US, de-DE..");

            args.add_argument()
                .names({ L"-c", L"--count" })
                .description(L"Suggestions output count: 0-1000");

            args.add_argument()
                .names({ L"-a", L"--auto" })
                .description(L"Spell check, automatic text update, default option: disabled.");

            args.add_argument()
                .names({ L"-p", L"--poinput" })
                .description(L"PO/POT input file, full path")
                .required(true);
            args.add_argument()
                .names({ L"-o", L"--pooutput" })
                .description(L"PO/POT output file, full path, optionals");

            args.add_argument()
                .names({ L"-r", L"--reverse" })
                .description(L"Change the primary language for the POT input file");


            args.add_argument()
                .names({ L"-e", L"--exclude" })
                .description(L"By default, exclude file it is in the directory of the executable, and name 'po2spell.<Microsoft language ID>.exclude'");

            args.enable_help();

            auto err = args.parse(argc, argv);
            if (args.exists(L"s")) {
                std::unique_ptr<SPELL::Corrector> sc = std::make_unique<SPELL::Corrector>();
                sc->Init();
                std::vector<std::wstring> list = sc->List();
                sc.reset();

                uint16_t line{ 0 };
                for (auto& a : list) {
                    if ((!line) || (line++ > 10)) {
                        line = 1;
                        cw.print(L"\n\t\t");
                    }
                    cw.print((std::wstringstream() << a.c_str() << L", "));
                }
                cw.print(L"\n\n");
                return 0;
            }
            else if (err) {
                cw.print(info::configinfo::header());
                cw.print((std::wstringstream() << err.what() << L"\n"));
                args_using();
                return 0;
            }
            else if (args.exists(L"help")) {
                cw.print(info::configinfo::header(true));
                args.print_help();
                args_using();
                return 0;
            }
            cnf = std::make_shared<config>(args);
        }

       if (cnf->empty()) {
           cw.print(info::configinfo::header());
           cw.print(L"\n\t! Input file path is empty!\n\n");
           args_using();
           return 0;
       }

        cw.print((std::wstringstream()
            << L"\n\t* Process file: " << cnf->path_po.filename().wstring()
            << L", check language: " << cnf->lang_id
            << L", suggestions: " << ((cnf->sug_count < 0) ? 50 : cnf->sug_count)
            << L", reverse: " << std::boolalpha << cnf->is_reverse << L"\n")
        );
        if (!cnf->path_exclude.empty())
            cw.print((std::wstringstream() << L"\t* Usiing exclude file: " << cnf->path_exclude.wstring() << L"\n"));

        cnf->begin();
        std::unique_ptr<writer> ptr = std::make_unique<writer>(cnf);
        ptr->parse();
        cw.print(cnf->end());

    } catch (...) {
        cw.print_exception(std::current_exception(), __FUNCTIONW__);
    }
    return 0;
}
