/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    po2rc LANGUAGE PO/POT to RC file conversion

    See README.md for more details.
*/

#include "global.h"
#include "RCParser.h"
#include "Xml.h"
#include "Lang.h"

    class config : public info::configinfo {
    public:

        std::wstring lang_tmpl{};
        std::wstring lang_out{};
        std::wstring lang_id{};

        std::filesystem::path path_xlf{};
        std::filesystem::path path_out{};
        std::filesystem::path path_tmpl{};
        std::filesystem::path path_exclude{};

        bool is_normalize;

        config(argparse::ArgumentParser& args) {

            if (args.exists(L"s")) lang_tmpl = args.get<std::wstring>(L"s");
            if (args.exists(L"l")) lang_out = args.get<std::wstring>(L"l");
            if (args.exists(L"i")) lang_id = args.get<std::wstring>(L"i");
            if (lang_id.empty()) {
                LANG::Langid lang{};
                lang_id = lang.find(lang_out);
            } else {
                lang_id = LANG::LangUtil::check_sublang(lang_id);
            }

            if (args.exists(L"e")) path_exclude = std::filesystem::path(args.get<std::wstring>(L"e"));
            if (args.exists(L"t")) path_tmpl = std::filesystem::path(args.get<std::wstring>(L"t"));
            if (args.exists(L"x")) path_xlf = std::filesystem::path(args.get<std::wstring>(L"x"));

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

            if (!std::filesystem::exists(path_xlf)) path_xlf = std::filesystem::path();
            if (!std::filesystem::exists(path_tmpl)) path_tmpl = std::filesystem::path();
            if (!std::filesystem::exists(path_out.root_directory())) path_out = std::filesystem::path();
        }

        const bool empty() {
            return path_tmpl.empty() || path_xlf.empty() || path_out.empty() || lang_id.empty();
        }
    };

    class writer {
    private:

        std::wifstream stream_in_{};
        std::wofstream stream_out_{};
        PARSERS::RCParser<CONFIG> parser_;
        CONFIG& config_;

    public:
        writer(CONFIG& c) : config_(c), parser_(PARSERS::RCParser<CONFIG>(c)) {
        }
        ~writer() {
            close();
        }
        const bool parse() {
            try {

                /* parse XLIFF file */
                {
                    std::wifstream fxlf(config_->path_xlf.wstring(), std::ios::in | std::ios::binary);
                    if (!fxlf.is_open())
                        throw std::runtime_error("error open XLIFF file");

                    fxlf.imbue(std::locale(".utf-8"));
                    {
                        xml::Xml root{};
                        root.load<std::filesystem::path>(config_->path_xlf);
                        xml::Xml& f = root[L"file"];
                        xml::Xml& b = f[L"body"];

                        for (xml::Xml::xiterator i = b.begin(); i != b.end(); i++) {

                            if (i->name()._Equal(L"trans-unit")) {
                                xml::Xml& a = *static_cast<xml::Xml*>(&*i);
                                parser_.add_dictionary(a[L"source"].text(), utils::po_normalize(a[L"target"].text()));
                            }
                        }
                        root.clear();
                    }
                    fxlf.close();

                    if (parser_.empty_dictionary())
                        throw std::runtime_error("error, XLIFF dictionary empty");
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
        << L" -s RU -l ZU -i LANG_ZULU\n\t\t-x y:\\path\\to\\file\\resource.po -t x:\\path\\to\\file\\template.rc -d x:\\path\\output\\directory\n\t\tor:")
    );
    cw.print((std::wstringstream()
        << L"\n\t" << info::configinfo::File
        << L" -l ZU\n\t\t-x y:\\path\\to\\file\\resource.xlf -t x:\\path\\to\\file\\template.rc -o x:\\path\\output\\resource.rc\n\n")
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
                .description(L"Output language, two chars: EN,DE,de-DE..")
                .required(true);
            args.add_argument()
                .names({ L"-i", L"--langid" })
                .description(L"Output language Microsoft ID: LANG_*, optional, absolute");

            args.add_argument()
                .names({ L"-n", L"--normalize" })
                .description(L"normalize translated text");

            args.add_argument()
                .names({ L"-x", L"--xliffinput" })
                .description(L"XLIFF input file, full path")
                .required(true);
            args.add_argument()
                .names({ L"-t", L"--template" })
                .description(L"RC input template file, full path")
                .required(true);

            args.add_argument()
                .names({ L"-o", L"--rcoutput" })
                .description(L"RC output file, full path, or use -d option");
            args.add_argument()
                .names({ L"-d", L"--directory" })
                .description(L"RC output directory, required -s and -l options, full path, or use -o option");
            args.add_argument()
                .names({ L"-e", L"--exclude" })
                .description(L"By default, exclude file it is in the directory of the executable, and name 'xlf2rc.exclude'");

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
            << L"\n\t* Process file: " << cnf->path_xlf.filename().wstring()
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
