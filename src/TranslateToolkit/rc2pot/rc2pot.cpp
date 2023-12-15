/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    rc2pot LANGUAGE RC to PO/POT file conversion

    See README.md for more details.
*/

#include "global.h"

    class config : public info::configinfo {
    public:

        std::filesystem::path path_out{};
        std::filesystem::path path_dup{};
        std::filesystem::path path_rc{};
        std::filesystem::path path_exclude{};

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"r")) path_rc = std::filesystem::path(args.get<std::wstring>(L"r"));
            if (args.exists(L"e")) path_exclude = std::filesystem::path(args.get<std::wstring>(L"e"));

            std::wstring s = args.get<std::wstring>(L"o");
            if (s.empty()) {
                s = args.get<std::wstring>(L"d");
                if (s.empty()) return;

                path_out = std::filesystem::path(s);
                if (!std::filesystem::is_directory(path_out)) {
                    cw.print((std::wstringstream() << L"\n! Output path is not directory: " << path_out.wstring() << L"\n"));
                    return;
                }

                path_out.append(path_rc.filename().wstring());
                path_out.replace_extension(L".pot");
            }
            else path_out = std::filesystem::path(s);

            if (!std::filesystem::exists(path_rc)) path_rc = std::filesystem::path();
            if (!std::filesystem::exists(path_out.root_directory())) path_out = std::filesystem::path();
            else {
                path_dup = std::filesystem::path(path_out);
                path_dup.replace_extension(L".dup");
            }
        }

        const bool empty() {
            return path_rc.empty() || path_out.empty();
        }
    };

    class parser {
    public:
        enum class Types : uint8_t {
            NONE_ = 0,
            HIDE_,
            HTTP_,
            HTTPS_,
            ONE_,
            BLANK_,
            STATIC_,
            BUTTON_,
            SYSLISTVIEW_,
            TRACKBAR_,
            POPUP,
            MENUITEM,
            STRINGTABLE,
            BEGIN,
            END,
            CONTROL,
            PUSHBUTTON,
            GROUPBOX,
            LTEXT,
            RTEXT,
            CAPTION
        };
        static constexpr std::wstring_view Names[] = {
            L""sv,
            /* remove start with */
            L"__"sv,
            L"http://"sv,
            L"https://"sv,
            /* remove equals */
            L"-"sv,
            L" "sv,
            L"Static"sv,
            L"Button"sv,
            L"SysListView32"sv,
            L"msctls_trackbar32"sv,
            /* parse tokens */
            L"POPUP"sv,
            L"MENUITEM"sv,
            L"STRINGTABLE"sv,
            L"BEGIN"sv,
            L"END"sv,
            L"CONTROL"sv,
            L"PUSHBUTTON"sv,
            L"GROUPBOX"sv,
            L"LTEXT"sv,
            L"RTEXT"sv,
            L"CAPTION"sv
        };
    private:

        Types types_{ Types::NONE_ };
        EXCLUDELIST exclude_list_{};
        CONFIG& config_;

        std::wstring shift_(std::wstring& s) {
            try {
                if (s.empty() || (s.at(0) == 65279)) return std::wstring();
                size_t z = s.find_first_not_of(separators::all);
                if (z < s.length())
                    return s.substr(z);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring();
        }
        std::wstring shift_to_text_(std::wstring& s) {
            try {
                std::wstring line_{};
                int32_t begin_ = -1;

                for (uint32_t i = 0; i < s.length(); i++) {
                    if (s.at(i) == L'"') {
                        if (begin_ == -1) {
                            begin_ = (i + 1);
                            continue;
                        }
                        if (int32_t n = (i - begin_); n > 1)
                            return s.substr(begin_, n);
                        else break;
                    }
                }
            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring();
        }

        std::wstring extract_text_(std::wstring& s, size_t length) {
            try {
                const auto c = (s.length() > length) ? s.at(length) : 0;

                if ((c == 0) || (c == L'\0') || (c == L'\r') || (c == L'\n')) {}
                else if ((c == L' ') || (c == L',') || (c == L'\t') || (c == L'\v')) {

                    std::wstring line_ = s.substr(length);
                    line_ = shift_to_text_(line_);
                    
                    if (line_.empty())
                        return std::wstring();

                    for (size_t i = static_cast<size_t>(parser::Types::HIDE_);
                                i < static_cast<size_t>(parser::Types::POPUP); i++) {
                        const auto& d = parser::Names[i];

                        switch (static_cast<parser::Types>(i)) {
                            using enum parser::Types;
                            case HIDE_:
                            case HTTP_:
                            case HTTPS_: {
                                if (line_.starts_with(d.data())) return std::wstring();
                                break;
                            }
                            default: {
                                if (line_._Equal(d.data())) return std::wstring();
                                break;
                            }
                        }
                    }
                    for (auto& a : exclude_list_)
                        if (line_._Equal(a)) return std::wstring();

                    if (utils::is_wdigit(line_)) {
                        return std::wstring();
                    }
                    return line_;
                }
            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring();
        }
        std::wstring extract_stringtable_(std::wstring& s) {
            try {
                std::wstring line_{};

                line_ = shift_(s);
                if (line_.empty()) return std::wstring();

                size_t z = line_.find_first_of(separators::valid);
                if (z < line_.length())
                    return extract_text_(line_, z);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring();
        }

    public:

        parser(CONFIG& c) : config_(c) {
            utils::build_exclude_list(exclude_list_, config_->path_exclude);
        }

        std::wstring parse(std::wstring& line) {
            try {
                std::wstring line_ = shift_(line);
                if (line_.empty()) return std::wstring();

                if (types_ == Types::STRINGTABLE) {
                    std::wstring line__ = extract_stringtable_(line_);
                    if (!line__.empty()) return line__;
                }

                for (size_t i = static_cast<size_t>(parser::Types::POPUP); i < std::size(parser::Names); i++) {
                    const auto& data = parser::Names[i];

                    if (line_.starts_with(data.data())) {
                        
                        switch (static_cast<Types>(i)) {
                            using enum parser::Types;
                            case POPUP:
                            case LTEXT:
                            case RTEXT:
                            case CONTROL:
                            case CAPTION:
                            case MENUITEM:
                            case GROUPBOX:
                            case PUSHBUTTON: {

                                types_ = Types::NONE_;
                                return extract_text_(line_, data.length());
                            }
                            case BEGIN: {
                                if (types_ != Types::STRINGTABLE)
                                    types_ = Types::NONE_;
                                return std::wstring();
                            }
                            case STRINGTABLE: {
                                types_ = Types::STRINGTABLE;
                                return std::wstring();
                            }
                            case ONE_:
                            case NONE_:
                            case HIDE_:
                            case BLANK_:
                            case END: {
                                types_ = Types::NONE_;
                                return std::wstring();
                            }
                            default: break;
                        }
                    }
                }
            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return std::wstring();
        }
        std::wstring header(std::filesystem::path p) {
            return utils::po_header(config_->AppFile(), p);
        }
    };

    class writer {
    private:

        HASHLIST hash_list{};

        std::wofstream stream_out_{};
        std::wofstream stream_dup_{};
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

                stream_in_.open(config_->path_rc.wstring(), std::ios::in | std::ios::binary);
                if (!stream_in_.is_open()) return false;

                stream_out_.open(config_->path_out.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_out_.is_open()) return false;

                stream_dup_.open(config_->path_dup.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_dup_.is_open()) return false;

                stream_in_.imbue(std::locale(".utf-8"));
                stream_out_.imbue(std::locale(".utf-8"));
                stream_dup_.imbue(std::locale(".utf-8"));

                write(parser_.header(config_->path_rc));

                std::wstring line{};
                while (std::getline(stream_in_, line)) {
                    const auto c = line.empty() ? 0 : line.at(0);
                    if ((c == 0) || (c == L'#') || (c == L'\r') || (c == L'\n')) continue;

                    std::wstring s = parser_.parse(line);
                    if (!s.empty()) {
                        auto p = hash_list.insert(s);
                        if (!p.second) write_dup(p.first->c_str());
                    }
                }

                if (hash_list.empty()) return false;
                for (auto& a : hash_list) write_format(a);

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return false;
        }
        void write(const std::wstring s) {
            stream_out_ << s;
        }
        void write(const std::wstringstream s) {
            stream_out_ << s.str();
        }
        void write_format(const std::wstring& s) {
            stream_out_ << utils::po_format(s);
        }
        void write_dup(const std::wstring& s) {
            stream_dup_ << s.c_str() << "\n";
        }
        void close() {
            if (stream_in_.is_open())
                stream_in_.close();
            if (stream_out_.is_open()) {
                stream_out_.flush();
                stream_out_.close();
            }
            if (stream_dup_.is_open()) {
                stream_dup_.flush();
                stream_dup_.close();
            }
        }
    };

void args_using() {
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -r x:\\path\\to\\file\\resource.rc -o x:\\path\\to\\file\\resource.pot\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -r x:\\path\\to\\file\\resource.rc -d x:\\path\\output\\directory\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {

        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-r", L"--rcinput" })
                .description(L"RC input file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-o", L"--output" })
                .description(L"PO/POT output file, full path, or use -d option");
            args.add_argument()
                .names({ L"-d", L"--directory" })
                .description(L"PO/POT output directory, full path, or use -o option");
            args.add_argument()
                .names({ L"-e", L"--exclude" })
                .description(L"By default, exclude file it is in the directory of the executable, and name 'rc2pot.exclude'");

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
            cw.print(L"\n\t! Input/Output path, directory or file is empty!\n\n");
            args_using();
            return 0;
        }

        cw.print((std::wstringstream()
            << L"\n\t* Process file: " << cnf->path_rc.filename().wstring()
            << L", out: " << cnf->path_out.filename().wstring()
            << L"\n")
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
