/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    po2rc LANGUAGE PO/POT to RC file conversion

    See README.md for more details.
*/

#include "global.h"

    class config : public info::configinfo {
    public:

        std::wstring lang_tmpl{};
        std::wstring lang_out{};
        std::wstring lang_id{};

        std::filesystem::path path_out{};
        std::filesystem::path path_po{};
        std::filesystem::path path_tmpl{};
        std::filesystem::path path_exclude{};

        bool is_normalize;

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"s")) lang_tmpl = args.get<std::wstring>(L"s");
            if (args.exists(L"l")) lang_out = args.get<std::wstring>(L"l");
            if (args.exists(L"i")) lang_id = args.get<std::wstring>(L"i");

            if (args.exists(L"e")) path_exclude = std::filesystem::path(args.get<std::wstring>(L"e"));
            if (args.exists(L"t")) path_tmpl = std::filesystem::path(args.get<std::wstring>(L"t"));
            if (args.exists(L"p")) path_po = std::filesystem::path(args.get<std::wstring>(L"p"));

            is_normalize = args.exists(L"n");

            std::wstring s = args.get<std::wstring>(L"o");
            if (s.empty()) {
                s = args.get<std::wstring>(L"d");
                if (s.empty()) return;

                path_out = std::filesystem::path(s);
                if (!std::filesystem::is_directory(path_out)) {
                    cw.print((std::wstringstream() << L"\n! Output path is not directory: " << path_out.wstring() << L"\n"));
                    return;
                }

                std::wstring out_ = path_tmpl.filename().wstring();
                if (!lang_tmpl.empty() && !lang_out.empty()) {

                    std::size_t pos = out_.find((std::wstringstream() << L"x" << lang_tmpl.c_str()).str());
                    if (pos != std::wstring::npos) path_out.append(out_.replace((pos + 1), 2, lang_out.c_str()));
                    else path_out.append(out_);
                } else path_out.append(out_);
            } else path_out = std::filesystem::path(s);

            if (!std::filesystem::exists(path_po)) path_po = std::filesystem::path();
            if (!std::filesystem::exists(path_tmpl)) path_tmpl = std::filesystem::path();
            if (!std::filesystem::exists(path_out.root_directory())) path_out = std::filesystem::path();
        }

        const bool empty() {
            return path_tmpl.empty() || path_po.empty() || path_out.empty();
        }
    };

    class parser {
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

        CONFIG& config_;
        LANGLIST dictionary_{};
        EXCLUDELIST exclude_list_{};

    public:

        parser(CONFIG& c) : config_(c) {
            utils::build_exclude_list(exclude_list_, config_->path_exclude);
        }

        std::wstring parse(std::wstring& line) {
            try {
                if (line.empty())
                    return std::wstring();

                do {
                    const auto c = line.at(0);
                    if (c == L'L') {
                        if (line.starts_with(L"LANGUAGE ") && !config_->lang_id.empty())
                            return (std::wstringstream() << L"LANGUAGE " << config_->lang_id.c_str() << L", SUBLANG_DEFAULT\n").str();
                    }

                    for (auto& a : Names)
                        if (line.starts_with(a)) return std::wstring(line);

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

                    LANGP& li = *static_cast<LANGP*>(&*it);
                    return line.replace((n1 + 1), line_.length(), li.second);

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

            if (config_->is_normalize && (trans.length() > 1)) {
                /* “ texts ” */
                size_t pos{ 0 };
                std::wstring& t(trans);
                while (pos != std::wstring::npos) {
                    pos = t.find_first_of(separators::text, pos);
                    if (pos == std::wstring::npos) break;
                    t = t.replace(pos, 1, L"“");

                    pos = t.find_first_of(separators::text, (pos + 1));
                    if (pos == std::wstring::npos) break;
                    t = t.replace(pos++, 1, L"”");
                }
                /* "& texts */
                if (t.at(0) == separators::command[0] && t.starts_with(separators::command))
                    t = t.replace(0, 2, L"&");

                dictionary_.push_back(std::make_pair(orig, t));
            } else dictionary_.push_back(std::make_pair(orig, trans));
        }
    };

    class writer {
    private:

        std::wofstream stream_out_{};
        std::wifstream stream_in_{};
        parser parser_;
        CONFIG& config_;

    public:
        writer(CONFIG& c) : config_(c), parser_(parser(c)) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                /* parse POT file */
                {
                    std::wifstream fpot(config_->path_po.wstring(), std::ios::in | std::ios::binary);
                    if (!fpot.is_open())
                        throw std::runtime_error("error open PO file");

                    fpot.imbue(std::locale(".utf-8"));
                    {
                        std::wstring line{};
                        std::wstring line1{};
                        std::wstring line2{};
                        while (std::getline(fpot, line)) {
                            if (line.empty() || (line.at(0) != L'm')) continue;

                            size_t b = line.find_first_of(L'"');
                            if (b == std::wstring::npos) continue;
                            size_t e = line.find_last_of(L'"');
                            if (e == std::wstring::npos) continue;

                            std::wstring msgstr = line.substr((b + 1), (e - b - 1));
                            if (msgstr.empty()) continue;

                            if (line.starts_with(L"msgid")) {
                                line1 = msgstr;
                                line2 = L"";
                                continue;
                            }
                            if (line.starts_with(L"msgstr"))
                                line2 = msgstr;

                            if (!line1.empty() && !line2.empty()) {
                                parser_.add_dictionary(line1, line2);
                                line1 = line2 = L"";
                            }
                        }
                    }
                    fpot.close();

                    if (parser_.empty_dictionary())
                        throw std::runtime_error("error, PO dictionary empty");
                }

                stream_in_.open(config_->path_tmpl.wstring(), std::ios::in | std::ios::binary);
                if (!stream_in_.is_open()) return false;

                stream_out_.open(config_->path_out.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_out_.is_open()) return false;

                stream_in_.imbue(std::locale(".utf-8"));
                stream_out_.imbue(std::locale(".utf-8"));

                /* read RC template && wrie RC output */
                {
                    std::wstring line{};
                    while (std::getline(stream_in_, line)) {
                        if (line.empty()) continue;
                        const auto c = line.at(0);
                        if ((c == 0) || (c == L'#') || (c == L'/') || (c == L'\r') || (c == L'\n') || (c == 65279))
                            write(line, true);
                        else
                            write(parser_.parse(line), true);
                    }
                }

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return false;
        }
        void write(const std::wstring s, bool crlf = false) {
            stream_out_ << s;
            if (crlf) stream_out_ << L"\n";
        }
        void write(const std::wstringstream s) {
            stream_out_ << s.str();
        }
        void close() {
            if (stream_in_.is_open())
                stream_in_.close();
            if (stream_out_.is_open()) {
                stream_out_.flush();
                stream_out_.close();
            }
        }
    };

void args_using() {
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -s RU -l ZU -i LANG_ZULU\n\t\t-p x:\\path\\to\\file\\resource.po -t x:\\path\\to\\file\\template.rc -d x:\\path\\output\\directory\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -i LANG_ZULU\n\t\t-p x:\\path\\to\\file\\resource.po -t x:\\path\\to\\file\\template.rc -o x:\\path\\output\\resource.rc\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {
        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-s", L"--langtemplate" })
                .description(L"RC template language, two chars: EN,RU..");
            args.add_argument()
                .names({ L"-l", L"--langoutput" })
                .description(L"Output language, two chars: EN,DE..");
            args.add_argument()
                .names({ L"-i", L"--langid" })
                .description(L"Output language Microsoft ID: LANG_*")
                .required(true);

            args.add_argument()
                .names({ L"-n", L"--normalize" })
                .description(L"normalize translated text");

            args.add_argument()
                .names({ L"-p", L"--poinput" })
                .description(L"PO input file, full path")
                .required(true);
            args.add_argument()
                .names({ L"-t", L"--template" })
                .description(L"RC input template file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-o", L"--output" })
                .description(L"RC output file, full path, or use -d option");
            args.add_argument()
                .names({ L"-d", L"--directory" })
                .description(L"RC output directory, required -s and -l options, full path, or use -o option");
            args.add_argument()
                .names({ L"-e", L"--exclude" })
                .description(L"By default, exclude file it is in the directory of the executable, and name 'po2rc.exclude'");

            args.enable_help();

            auto err = args.parse(argc, argv);
            if (err) {
                cw.print(info::configinfo::header());
                cw.print((std::wstringstream() << err.what() << L"\n"));
                args_using();
                return 0;
            }
            if (args.exists(L"help")) {
                cw.print(info::configinfo::header(true));
                args.print_help();
                args_using();
                return 0;
            }
            cnf = std::make_shared<config>(args);
        }

       if (cnf->empty()) {
           cw.print(info::configinfo::header());
           cw.print(L"\n\t! Output path, directory or file is empty!\n\n");
           args_using();
           return 0;
       }

        cw.print((std::wstringstream()
            << L"\n\t* Process file: " << cnf->path_po.filename().wstring()
            << L", template: " << cnf->lang_tmpl
            << L", to language: " << cnf->lang_out
            << L", normalize: " << std::boolalpha << cnf->is_normalize << L"\n")
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
