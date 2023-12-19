/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    xlf2po LANGUAGE XLIFF to PO/POT file conversion

    See README.md for more details.
*/

#include "global.h"
#include "Xml.h"

    class config : public info::configinfo {
    public:

        std::filesystem::path path_spell{};
        std::filesystem::path path_xlf{};

        bool is_pot;
        bool is_reverse;
        bool is_normalize;

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"x")) path_xlf = std::filesystem::path(args.get<std::wstring>(L"x"));
            is_pot = args.exists(L"p");
            is_reverse = args.exists(L"r");
            is_normalize = args.exists(L"n");

            std::wstring s = args.get<std::wstring>(L"o");
            if (s.empty()) {
                s = args.get<std::wstring>(L"d");
                if (s.empty()) return;

                path_spell = std::filesystem::path(s);
                if (!std::filesystem::is_directory(path_spell)) {
                    cw.print((std::wstringstream() << L"\n! Output path is not directory: " << path_spell.wstring() << L"\n"));
                    return;
                }

                path_spell.append(path_xlf.filename().wstring());
                path_spell.replace_extension(L".po");
            }
            else path_spell = std::filesystem::path(s);

            if (!std::filesystem::exists(path_xlf)) path_xlf = std::filesystem::path();
            if (!std::filesystem::exists(path_spell.root_directory())) path_spell = std::filesystem::path();
        }

        const bool empty() {
            return path_xlf.empty() || path_spell.empty();
        }
    };

    class writer {
    private:

        std::wofstream stream_spell_{};
        CONFIG& config_;

    public:
        writer(CONFIG& c) : config_(c) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                xml::Xml root{};
                root.load<std::filesystem::path>(config_->path_xlf);
                xml::Xml& f = root[L"file"];
                xml::Xml& b = f[L"body"];

                stream_spell_.open(config_->path_spell.wstring(), std::ios_base::binary | std::ios_base::out | std::ios::trunc);
                if (!stream_spell_.is_open()) return false;
                stream_spell_.imbue(std::locale(".utf-8"));

                write(utils::po_header(config_->AppFile(), config_->path_xlf));

                for (xml::Xml::xiterator i = b.begin(); i != b.end(); i++) {

                    if (i->name()._Equal(L"trans-unit")) {
                        xml::Xml& a = *static_cast<xml::Xml*>(&*i);
                        if (config_->is_pot) {
                            if (config_->is_reverse)
                                write_format((config_->is_normalize ? utils::po_normalize(a[L"target"].text()) : a[L"target"].text()), L"");
                            else write_format(a[L"source"].text(), L"");
                        }
                        else write_format(a[L"source"].text(), (config_->is_normalize ? utils::po_normalize(a[L"target"].text()) : a[L"target"].text()));
                    }
                }

                close();
                root.clear();

            } catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return false;
        }
        void write(const std::wstring s) {
            stream_spell_ << s;
        }
        void write(const std::wstringstream s) {
            stream_spell_ << s.str();
        }
        void write_format(const std::wstring& s, const std::wstring& t) {
            stream_spell_ << utils::po_format(s, t);
        }
        void close() {
            if (stream_spell_.is_open()) {
                stream_spell_.flush();
                stream_spell_.close();
            }
        }
    };

void args_using() {
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -x y:\\path\\to\\file\\resource.xlf -o x:\\path\\to\\file\\resource.po\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -x y:\\path\\to\\file\\resource.xlf -d x:\\path\\output\\directory\n\n")
    );
}

int wmain(int argc, const wchar_t* argv[]) {

    try {

        CONFIG cnf{};
        {
            argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
            args.add_argument()
                .names({ L"-x", L"--xliffinput" })
                .description(L"XLIFF input file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-p", L"--pot" })
                .description(L"output POT file, 'msgstr' empty");

            args.add_argument()
                .names({ L"-r", L"--reverse" })
                .description(L"Change primary language to output POT file, required -p option");

            args.add_argument()
                .names({ L"-n", L"--normalize" })
                .description(L"normalize translated text");

            args.add_argument()
                .names({ L"-o", L"--output" })
                .description(L"PO/POT output file, full path, or use -d option");
            args.add_argument()
                .names({ L"-d", L"--directory" })
                .description(L"PO/POT output directory, full path, or use -o option");

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
            << L"\n\t* Process file: " << cnf->path_xlf.filename().wstring()
            << L", out: " << cnf->path_spell.filename().wstring()
            << L", normalize: " << std::boolalpha << cnf->is_normalize << L"\n")
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
