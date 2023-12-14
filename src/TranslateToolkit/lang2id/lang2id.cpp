/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT

    po2rc LANGUAGE PO/POT to RC file conversion

	Microsoft: Valid locale identifiers:
	https://learn.microsoft.com/en-us/previous-versions/windows/desktop/indexsrv/valid-locale-identifiers

    See README.md for more details.
*/

#include "global.h"

using namespace std;

struct LANGBASE {
public:
	const LANGID id;
	std::wstring_view name;
	LANGBASE(uint32_t i, const std::wstring_view s) : id(i), name(std::wstring_view(s)) {}
};

std::vector<LANGBASE*> base;

void add(uint32_t id, const std::wstring_view s) {
	base.push_back(new LANGBASE(id, s));
}

void args_using() {
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" LANG_GERMAN\n\t\tor:")
	);
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" German\n\t\tor:")
	);
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" DE\n\t\tor:")
	);
	cw.print((std::wstringstream()
		<< L"\n\t" << info::configinfo::File
		<< L" de-DE\n\n")
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
			if (args.exists(L"help") || (argc < 1)) {
				cw.print(info::configinfo::header(true));
				args.print_help();
				args_using();
				return 0;
			}
		}
		auto s = std::wstring(argv[1]);
		if (s.empty()) return 0;
		
		#include "lang2id.h"

		for (auto a : base) {
			if (a->name._Equal(s)) {
				std::cout << std::setw(8) << std::setfill('0') << (uint32_t)a->id;
				return 0;
			}
		}
	}
	catch (...) {
		cw.print_exception(std::current_exception(), __FUNCTIONW__);
	}
	return 0;
}
