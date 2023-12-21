/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    id2lang LANGUAGE NLS string to Microsoft Language and Sub Language ID
	Output: "LANG_*, SUBLANG_*"

	Microsoft: Valid locale identifiers:
	https://learn.microsoft.com/en-us/previous-versions/windows/desktop/indexsrv/valid-locale-identifiers

    See README.md for more details.
*/

#include "global.h"
#include "Lang.h"

void args_using() {
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" DE\n\t\tor:")
	);
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" de-DE\n\t\tor:")
	);
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" de_DE\n\n")
	);
}

int wmain(int argc, const wchar_t* argv[]) {
	try {
		{
			argparse::ArgumentParser args(info::configinfo::File, info::configinfo::Title);
			args.enable_help();
			auto err = args.parse(argc, argv);
			if (err) {
				cw.print(info::configinfo::header());
				cw.print((std::wstringstream() << err.what() << L"\n"));
				args_using();
				return 0;
			}
			if (args.exists(L"help") || (argc < 2)) {
				cw.print(info::configinfo::header(true));
				args.print_help();
				args_using();
				return 0;
			}
		}
		auto s = std::wstring(argv[1]);
		if (s.empty()) return 0;

		LANG::Langid lang{};
		std::wcout << lang.find(s);
	}
	catch (...) {
		cw.print_exception(std::current_exception(), __FUNCTIONW__);
	}
	return 0;
}
