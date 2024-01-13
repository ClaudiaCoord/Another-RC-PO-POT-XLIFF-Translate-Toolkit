/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    po2rc Different LANGUAGE PO/POT files

    See README.md for more details.
*/

#include "global.h"

    class config : public info::configinfo {
    public:

        std::filesystem::path path_old_po{};
        std::filesystem::path path_new_po{};
        std::filesystem::path path_out{};

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"o")) path_old_po = std::filesystem::path(args.get<std::wstring>(L"o"));
            if (args.exists(L"n")) path_new_po = std::filesystem::path(args.get<std::wstring>(L"n"));
            if (args.exists(L"p")) path_out = std::filesystem::path(args.get<std::wstring>(L"p"));

            if (!std::filesystem::exists(path_old_po)) path_old_po = std::filesystem::path();
            if (!std::filesystem::exists(path_new_po)) path_new_po = std::filesystem::path();
            if (!std::filesystem::exists(path_out.root_directory())) path_out = std::filesystem::path();
        }

        const bool empty() const {
            return path_old_po.empty() || path_new_po.empty() || path_out.empty();
        }
    };

    class parser {
    private:

        CONFIG& config_;
        EXCLUDELIST po_old_list_{};
        EXCLUDELIST po_new_list_{};

        void parse_stream_(std::wifstream& s, EXCLUDELIST& list) {
            try {
                std::wstring line{};
                while (std::getline(s, line)) {

                    if (line.empty() || (line.at(0) != L'm')) continue;
                    if (line.starts_with(L"msgid")) {
                        size_t n1 = line.find_first_of(separators::text);
                        if (n1 == std::wstring::npos) continue;
                        size_t n2 = line.find_first_of(separators::text, (n1 + 1));
                        if (n2 == std::wstring::npos) continue;

                        std::wstring line_ = line.substr((n1 + 1), (n2 - n1 - 1));
                        if (line_.empty() || (line_.at(0) == L'"')) continue;
                        list.push_back(line_);
                    }
                }
            }
            catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
        }

    public:

        parser(CONFIG& c) : config_(c) {
        }
        ~parser() {
        }

        bool parse_old_po(std::wifstream& s) {
            parse_stream_(s, po_old_list_);
            return !po_old_list_.empty();
        }
        bool parse_new_po(std::wifstream& s) {
            parse_stream_(s, po_new_list_);
            return !po_new_list_.empty();
        }
        EXCLUDELIST parse() {
            try {
                EXCLUDELIST list{};

                if ((po_old_list_.empty() && po_new_list_.empty()) || po_new_list_.empty())
                    return list;
                if (po_old_list_.empty())
                    return po_new_list_;

                for (auto& a : po_new_list_) {
                    bool found{ false };
                    for (auto& b : po_old_list_)
                        if (a._Equal(b)) { found = true; break; }
                    if (!found)
                        list.push_back(a);
                }
                return list;
            }
            catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return EXCLUDELIST();
        }
        std::wstring header(std::filesystem::path p) {
            return utils::po_header(config_->AppFile(), p);
        }
    };

    class writer {
    private:

        std::wofstream stream_out_{};
        parser parser_;
        CONFIG& config_;

    public:
        writer(CONFIG& c) : config_(c), parser_(c) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                std::wifstream fso(config_->path_old_po.wstring(), std::ios::in | std::ios::binary);
                if (!fso.is_open())
                    throw std::runtime_error("error open OLD PO file");

                std::wifstream fsn(config_->path_new_po.wstring(), std::ios::in | std::ios::binary);
                if (!fso.is_open())
                    throw std::runtime_error("error open NEW PO file");

                fso.imbue(std::locale(".utf-8"));
                fsn.imbue(std::locale(".utf-8"));
                {
                    parser_.parse_old_po(fso);
                    fso.close();
                    parser_.parse_new_po(fsn);
                    fsn.close();

                    EXCLUDELIST list = parser_.parse();
                    if (list.empty()) return false;

                    stream_out_.open(config_->path_out.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                    if (!stream_out_.is_open()) return false;

                    stream_out_.imbue(std::locale(".utf-8"));
                    write(parser_.header(config_->path_out));
                    for (auto& a : list) write_format(a);

                    close();
                    return true;
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
        void write_format(const std::wstring& s) {
            stream_out_ << utils::po_format(s);
        }
        void close() {
            if (stream_out_.is_open()) {
                stream_out_.flush();
                stream_out_.close();
            }
        }
    };

void args_using() {
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -o x:\\path\\to\\file\\old.po -n x:\\path\\to\\file\\new.po -p x:\\path\\output\\directory\\out.po\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {
        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-o", L"--oldpo" })
                .description(L"Old PO input file, full path")
                .required(true);
            args.add_argument()
                .names({ L"-n", L"--newpo" })
                .description(L"New PO input file, full path")
                .required(true);
            args.add_argument()
                .names({ L"-p", L"--outpo" })
                .description(L"Different PO output file, full path")
                .required(true);
                
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
           cw.print(L"\n\t! Check files path, files paths is empty!\n\n");
           args_using();
           return 0;
       }

        cw.print((std::wstringstream()
            << L"\n\t* Process ->"
            << L"\n\t\tOLD: " << cnf->path_old_po.filename().wstring()
            << L",\n\t\tNEW: " << cnf->path_new_po.filename().wstring()
            << L",\n\t\tOUT: " << cnf->path_out.filename().wstring() << L"\n")
        );

        cnf->begin();
        std::unique_ptr<writer> ptr = std::make_unique<writer>(cnf);
        ptr->parse();
        cw.print(cnf->end());

    } catch (...) {
        cw.print_exception(std::current_exception(), __FUNCTIONW__);
    }
    return 0;
}
