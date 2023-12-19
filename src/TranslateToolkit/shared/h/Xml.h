/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#pragma once

#include "XmlException.h"

namespace xml {

	class Value {
	private:
		std::wstring value_{};
	public:
		Value();
		Value(bool);
		Value(int);
		Value(double);
		Value(const wchar_t*);
		Value(const std::wstring&);
		~Value();

		Value& operator = (bool);
		Value& operator = (int);
		Value& operator = (double);
		Value& operator = (const wchar_t*);
		Value& operator = (const std::wstring&);
		Value& operator = (const Value&);

		bool operator == (const Value&);
		bool operator != (const Value&);

		operator bool();
		operator int();
		operator double();
		operator std::wstring();

		std::wstring& str();
	};

	class Xml {
	public:
		typedef std::list<Xml>::iterator xiterator;

	private:
		std::wstring name_{};
		std::wstring text_{};
		std::map<std::wstring, Value> attrs_{};
		std::list<Xml> child_{};

		const bool load_(const std::wstring&);
		const bool save_(const std::wstring&);

	public:
		Xml();
		Xml(const std::wstring&);
		Xml(const Xml&);

		std::wstring& name();
		void name(const std::wstring&);

		std::wstring& text();
		void text(const std::wstring&);

		Value& attr(const std::wstring&);
		void attr(const std::wstring&, const Value&);

		Xml& operator = (const Xml&);
		Xml& operator [] (int);
		Xml& operator [] (size_t);
		Xml& operator [] (const wchar_t*);
		Xml& operator [] (const std::wstring&);

		void append(const Xml&);
		void remove(int);
		void remove(size_t);
		void remove(const wchar_t*);
		void remove(const std::wstring&);

		size_t size() const;
		void clear();
		std::wstring str();
		const bool empty() const;

		xiterator begin();
		xiterator end();
		xiterator erase(xiterator it);

		static Xml parse(const std::wstring&);

		template <typename T1>
		const bool load(const T1& s) {
			if constexpr (std::is_same_v<std::wstring, T1>) {
				return load_(s);
			}
			else if constexpr (std::is_same_v<std::string, T1>) {
				std::wstring w = std::wstring(s.begin(), s.end());
				return load_(s);
			}
			else if constexpr (std::is_same_v<std::filesystem::path, T1>) {
				std::wstring w = s.wstring();
				return load_(w);
			}
			return false;
		}

		template <typename T1>
		const bool save(const T1& s) {
			if constexpr (std::is_same_v<std::wstring, T1>) {
				return save_(s);
			}
			else if constexpr (std::is_same_v<std::string, T1>) {
				std::wstring w = std::wstring(s.begin(), s.end());
				return save_(s);
			}
			else if constexpr (std::is_same_v<std::filesystem::path, T1>) {
				std::wstring w = s.wstring();
				return save_(w);
			}
			return false;
		}
	};

	class Parser {
	private:
		std::wstring str_{};
		std::size_t idx_{ 0 };

		void skip_white_space_();
		bool parse_declaration_();
		bool parse_comment_();
		Xml parse_element_();
		std::wstring parse_element_name_();
		std::wstring parse_element_text_();
		std::wstring parse_element_attr_key_();
		std::wstring parse_element_attr_val_();

	public:
		Parser() = default;
		~Parser() = default;
		bool load_file(const std::wstring&);
		bool load_string(const std::wstring&);
		Xml parse(bool = false);
	};

}
