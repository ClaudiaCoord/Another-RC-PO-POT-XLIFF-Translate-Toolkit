/*
	Another RC/PO/POT/XLIFF Translate Toolkit.
	(c) CC 2023, MIT
	See README.md for more details.
*/

#pragma once

using namespace std::string_view_literals;
/*
	Another RC/PO/POT/XLIFF Translate Toolkit.
	(c) CC 2023, MIT
	See README.md for more details.
*/

namespace info {

	#include "app_info.h"
	#define SW__(A) L ## A ## sv
	#define SW_(A) SW__(A)

	class configinfo {
	protected:
		std::chrono::steady_clock::time_point start_{};

	public:

		static constexpr std::wstring_view Cc = SW_(FILE_COPYRIGHT);
		static constexpr std::wstring_view File = SW_(FILE_NAME);
		static constexpr std::wstring_view Title = SW_(FILE_DESC);
		static constexpr std::wstring_view Product = SW_(FILE_PRODUCT);

		configinfo() = default;
		virtual ~configinfo() = default;

		const std::wstring AppCc() const {
			return Cc.data();
		}
		const std::wstring AppFile() const {
			return File.data();
		}
		const std::wstring AppProduct() const {
			return Product.data();
		}
		const std::wstring AppTitle() const {
			return Title.data();
		}
		static const std::wstring header(bool b = false) {
			return (std::wstringstream()
				<< info::configinfo::Product << L"\n"
				<< info::configinfo::Title << (b ? L"\n" : L"")
				<< (b ? info::configinfo::Cc : L"")
				<< L"\n\n").str();
		}

		void begin() {
			start_ = std::chrono::high_resolution_clock::now();
		}
		const std::wstring end() {
			auto t = std::chrono::high_resolution_clock::now();
			auto p = std::chrono::duration_cast<std::chrono::milliseconds>(t - start_);
			return std::format(L"\t= Running time: {:%T}\n\n", std::chrono::floor<std::chrono::milliseconds>(p));
	}
	};

}
