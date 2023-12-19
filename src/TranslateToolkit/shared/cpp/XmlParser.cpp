/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#define _NO_CONFIGINFO 1
#include "..\h\global.h"
#include "Xml.h"

namespace xml {

	bool Parser::load_file(const std::wstring& s) {
		std::wifstream f(s);
		if (f.fail())
			return false;

		f.imbue(std::locale(".utf-8"));

		std::wstringstream ss{};
		ss << f.rdbuf();
		str_ = ss.str();
		idx_ = (str_[0] == 65279) ? 1 : 0;
		return true;
	}
	bool Parser::load_string(const std::wstring& s) {
		str_ = s;
		idx_ = 0;
		return true;
	}

	Xml  Parser::parse(bool isstart)
	{
		skip_white_space_();
		if (isstart && !parse_declaration_())
			throw XmlException(L"parse declaration error");

		if (isstart)
			skip_white_space_();

		while (str_.compare(idx_, 4, L"<!--") == 0) {
			if (!parse_comment_())
				throw XmlException(L"parse comment error");

			skip_white_space_();
		}
		if (str_[idx_] == L'<' && (std::iswalpha(str_[idx_ + 1]) || (str_[idx_ + 1] == L'_')))
			return parse_element_();

		throw XmlException(L"parse element error");
	}
	Xml  Parser::parse_element_() {
		Xml elem{};
		idx_++;
		skip_white_space_();

		elem.name(parse_element_name_());
		const std::wstring name_end = L"</" + elem.name() + L">";

		while ((str_[idx_] != L'\0') && (str_.length() > idx_)) {

			skip_white_space_();
			if (str_[idx_] == L'/') {
				if (str_[idx_ + 1] == L'>') {
					idx_ += 2;
					break;
				}
				else throw XmlException(L"xml empty element is error");
			}
			else if (str_[idx_] == L'>') {
				idx_++;
				elem.text(parse_element_text_());
			}
			else if (str_[idx_] == L'<') {
				if (str_[idx_ + 1] == L'/') {
					size_t pos = str_.find(name_end, idx_);
					if (pos == std::wstring::npos)
						throw XmlException(L"xml element " + elem.name() + L" end tag not found");

					idx_ = (pos + name_end.size());
					break;
				}
				else if (str_.compare(idx_, 4, L"<!--") == 0) {
					if (!parse_comment_())
						throw XmlException(L"xml comment is error");
				}
				else elem.append(parse_element_());

			}
			else {

				std::wstring key = parse_element_attr_key_();
				skip_white_space_();
				if (str_[idx_] != L'=')
					throw XmlException(L"xml element attr is error " + key);

				idx_++;
				skip_white_space_();
				std::wstring val = parse_element_attr_val_();
				elem.attr(key, val);
			}
		}
		return elem;
	}
	bool Parser::parse_declaration_() {
		if (str_.compare(idx_, 5, L"<?xml") != 0)
			return false;

		idx_ += 5;
		size_t pos = str_.find(L"?>", idx_);
		if (pos == std::wstring::npos)
			return false;

		idx_ = pos + 2;
		return true;
	}
	bool Parser::parse_comment_() {
		if (str_.compare(idx_, 4, L"<!--") != 0)
			return false;

		idx_ += 4;
		size_t pos = str_.find(L"-->", idx_);
		if (pos == std::wstring::npos)
			return false;

		idx_ = pos + 3;
		return true;
	}
	void Parser::skip_white_space_() {
		idx_ = str_.find_first_not_of(L" \r\n\t", idx_);
	}

	std::wstring Parser::parse_element_name_() {
		size_t pos = idx_;
		if (std::iswalpha(str_[idx_]) || (str_[idx_] == L'_')) {
			idx_++;
			while (std::iswalnum(str_[idx_]) || (str_[idx_] == L'_') || (str_[idx_] == L'-') || (str_[idx_] == L':') || (str_[idx_] == L'.')) idx_++;
		}
		return str_.substr(pos, idx_ - pos);
	}
	std::wstring Parser::parse_element_text_() {
		size_t pos = idx_;
		while (str_[idx_] != L'<') idx_++;
		return str_.substr(pos, idx_ - pos);
	}
	std::wstring Parser::parse_element_attr_key_() {
		size_t pos = idx_;
		if (std::iswalpha(str_[idx_]) || (str_[idx_] == L'_')) {
			idx_++;
			while (std::iswalnum(str_[idx_]) || (str_[idx_] == L'_') || (str_[idx_] == L'-') || (str_[idx_] == L':')) idx_++;
		}
		return str_.substr(pos, idx_ - pos);
	}
	std::wstring Parser::parse_element_attr_val_() {
		if (str_[idx_] != L'"')
			throw XmlException(L"xml element attr value should be in double quotes");

		idx_++;
		size_t pos = idx_;
		while (str_[idx_] != L'"') idx_++;
		idx_++;
		return str_.substr(pos, idx_ - pos - 1);
	}
}
