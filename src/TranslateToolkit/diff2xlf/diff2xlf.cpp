/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    diff2xlf Different XLIFF source file to append XLIFF destination file

    See README.md for more details.
*/

#include "global.h"

    class config : public info::configinfo {
    public:

        std::filesystem::path path_src_xlf{};
        std::filesystem::path path_dst_xlf{};
        std::filesystem::path path_out_xlf{};

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"s")) path_src_xlf = std::filesystem::path(args.get<std::wstring>(L"s"));
            if (args.exists(L"d")) path_dst_xlf = std::filesystem::path(args.get<std::wstring>(L"d"));
            if (args.exists(L"o")) path_out_xlf = std::filesystem::path(args.get<std::wstring>(L"o"));

            if (!std::filesystem::exists(path_src_xlf)) path_src_xlf = std::filesystem::path();
            if (!std::filesystem::exists(path_dst_xlf)) path_dst_xlf = std::filesystem::path();
            if (!std::filesystem::exists(path_out_xlf.root_directory())) path_out_xlf = std::filesystem::path();
        }

        const bool empty() {
            return path_src_xlf.empty() || path_dst_xlf.empty() || path_out_xlf.empty();
        }
    };

    class parser {
    private:
        const bool find_body_(std::wstring& s, const wchar_t* sep) {
            size_t z = s.find_first_not_of(separators::all, 0);
            if (z != std::wstring::npos) {
                std::wstring s_(s);
                return s_.substr(z, s_.length() - z).starts_with(sep);
            }
            return s.starts_with(sep);
        }
    public:
        const bool find_body_begin(std::wstring& s) {
            return find_body_(s, separators::xliff_body_begin);
        }
        const bool find_body_end(std::wstring& s) {
            return find_body_(s, separators::xliff_body_end);
        }
    };

    class writer {
    private:

        std::wofstream stream_out_{};
        parser parser_{};
        CONFIG& config_;

    public:
        writer(CONFIG& c) : config_(c) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                std::wifstream fsrc(config_->path_src_xlf.wstring(), std::ios::in | std::ios::binary);
                if (!fsrc.is_open())
                    throw std::runtime_error("error open source XLIFF file");

                std::wifstream fdst(config_->path_dst_xlf.wstring(), std::ios::in | std::ios::binary);
                if (!fdst.is_open())
                    throw std::runtime_error("error open destination XLIFF file");

                stream_out_.open(config_->path_out_xlf.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_out_.is_open())
                    throw std::runtime_error("error open output XLIFF file");

                fsrc.imbue(std::locale(".utf-8"));
                fdst.imbue(std::locale(".utf-8"));
                stream_out_.imbue(std::locale(".utf-8"));
                {
                    std::wstring line{};
                    while (std::getline(fdst, line)) {
                        if (parser_.find_body_end(line)) break;
                        write(line);
                    }
                    fdst.close();

                    bool begin{ false };
                    while (std::getline(fsrc, line)) {

                        if (parser_.find_body_begin(line)) {
                            begin = true;
                            continue;
                        }
                        if (begin) write(line);
                    }
                    fsrc.close();
                }
                close();

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return false;
        }
        void write(const std::wstring s) {
            stream_out_ << s << L"\n";
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
        << L" -s x:\\path\\to\\file\\src.xlf -d x:\\path\\to\\file\\dst.xlf -o x:\\path\\to\\file\\output.xlf\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {

        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-s", L"--src" })
                .description(L"XLIFF source input file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-d", L"--dst" })
                .description(L"XLIFF destination input file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-o", L"--output" })
                .description(L"XLIFF output file, full path")
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
            << L"\n\t\tSource: " << cnf->path_src_xlf.filename().wstring()
            << L",\n\t\tDestination: " << cnf->path_dst_xlf.filename().wstring()
            << L",\n\t\tOutput: " << cnf->path_out_xlf.filename().wstring() << L"\n")
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
