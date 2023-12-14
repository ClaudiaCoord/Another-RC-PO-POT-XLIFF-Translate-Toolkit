/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <map>
#include <list>
#include <string>
#include <fstream>
#include <sstream>
#include <type_traits>

#include "Xml.h"

namespace xml {

	Value::Value() {
	}
	Value::Value(bool value) {
		*this = value;
	}
	Value::Value(int value) {
		*this = value;
	}
	Value::Value(double value) {
		*this = value;
	}
	Value::Value(const wchar_t* value) : value_(value) {
	}
	Value::Value(const std::wstring& value) : value_(value) {
	}
	Value::~Value() {
	}

	Value& Value::operator = (bool value) {
		value_ = value ? L"true" : L"false";
		return *this;
	}
	Value& Value::operator = (int value) {
		std::wstringstream ss;
		ss << value;
		value_ = ss.str();
		return *this;
	}
	Value& Value::operator = (double value) {
		std::wstringstream ss;
		ss << value;
		value_ = ss.str();
		return *this;
	}
	Value& Value::operator = (const wchar_t* value) {
		value_ = value;
		return *this;
	}
	Value& Value::operator = (const std::wstring& value) {
		value_ = value;
		return *this;
	}
	Value& Value::operator = (const Value& value) {
		value_ = value.value_;
		return *this;
	}
	bool   Value::operator == (const Value& other) {
		return value_ == other.value_;
	}
	bool   Value::operator != (const Value& other) {
		return !(value_ == other.value_);
	}

	Value::operator bool() {
		if (value_._Equal(L"true"))
			return true;
		else if (value_._Equal(L"false"))
			return false;
		return false;
	}
	Value::operator int() {
		return std::wcstol(value_.c_str(), nullptr, 10);
	}
	Value::operator double() {
		return std::wcstof(value_.c_str(), nullptr);
	}
	Value::operator std::wstring() {
		return value_;
	}
	std::wstring& Value::str() {
		return std::ref(value_);
	}

	Xml::Xml() {
	}
	Xml::Xml(const std::wstring& name) : name_(name) {
	}
	Xml::Xml(const Xml& a) 
		: name_(a.name_), text_(a.text_), attrs_(a.attrs_), child_(a.child_) {
	}

	std::wstring& Xml::name() {
		return std::ref(name_);
	}
	void Xml::name(const std::wstring& name) {
		name_ = std::wstring(name);
	}
	std::wstring& Xml::text() {
		return std::ref(text_);
	}
	void Xml::text(const std::wstring& text) {
		text_ = std::wstring(text);
	}
	Value& Xml::attr(const std::wstring& key) {
		return attrs_[key];
	}
	void Xml::attr(const std::wstring& key, const Value& value) {
		attrs_[key] = value;
	}

	Xml::xiterator Xml::begin() {
		return child_.begin();
	}
	Xml::xiterator Xml::end() {
		return child_.end();
	}
	Xml::xiterator Xml::erase(xiterator it) {
		it->clear();
		return child_.erase(it);
	}

	Xml& Xml::operator = (const Xml& clz) {
		clear();
		name_ = clz.name_;
		text_ = clz.text_;
		attrs_ = clz.attrs_;
		child_ = clz.child_;
		return *this;
	}

	Xml& Xml::operator [] (int idx) {
		return (*this)[static_cast<size_t>(idx)];
	}
	Xml& Xml::operator [] (size_t idx) {
		if (idx < 0)
			throw XmlException(L"idx less than zero");

		size_t size = child_.size();
		if ((idx >= 0) && (idx < size)) {
			auto it = child_.begin();
			for (size_t i = 0; i < idx; i++) it++;
			return *it;
		}
		if (idx >= size)
			for (size_t i = size; i < idx; i++)
				child_.push_back(Xml());

		return child_.back();
	}
	Xml& Xml::operator [] (const wchar_t* name) {
		return (*this)[std::wstring(name)];
	}
	Xml& Xml::operator [] (const std::wstring& name) {
		if (!child_.empty())
			for (auto& a : child_)
				if (a.name()._Equal(name)) return a;

		child_.push_back(Xml(name));
		return child_.back();
	}

	void Xml::append(const Xml& child) {
		child_.push_back(child);
	}
	void Xml::remove(int idx) {
		remove(static_cast<size_t>(idx));
	}
	void Xml::remove(size_t idx) {
		if (child_.empty()) return;

		size_t size = child_.size();
		if (idx < 0 || idx >= size) return;

		auto it = child_.begin();
		for (size_t i = 0; i < idx; i++) it++;

		it->clear();
		child_.erase(it);
	}
	void Xml::remove(const wchar_t* name) {
		return remove(std::wstring(name));
	}
	void Xml::remove(const std::wstring& name) {
		for (auto it = child_.begin(); it != child_.end();) {
			if (it->name()._Equal(name)) {
				it->clear();
				it = child_.erase(it);
			}
			else it++;
		}
	}

	size_t Xml::size() const {
		if (child_.empty()) return 0;
		return child_.size();
	}
	void Xml::clear() {

		name_ = std::wstring();
		text_ = std::wstring();
		attrs_.clear();

		if (!child_.empty())
			for (auto& a : child_) a.clear();
	}
	std::wstring Xml::str() {
		if (name_.empty()) return name_;

		std::wostringstream os{};
		os << L"<";
		os << name_;
		if (!attrs_.empty())
			for (auto& a : attrs_)
				os << L" " << a.first << L"=\"" << a.second.str() << L"\"";

		os << L">";
		if (!child_.empty())
			for (auto& a : child_)
				os << a.str();

		if (!text_.empty())
			os << text_;

		os << L"</" << name_ << L">";
		return os.str();
	}
	const bool Xml::empty() const {
		return child_.empty();
	}

	const bool Xml::load_(const std::wstring& s) {
		Parser p{};
		if (!p.load_file(s)) return false;

		*this = p.parse(true);
		return true;
	}
	const bool Xml::save_(const std::wstring& s) {
		std::wofstream f(s);
		if (f.fail()) return false;

		f.imbue(std::locale(".utf-8"));
		f << str();
		f.close();
		return true;
	}
	Xml Xml::parse(const std::wstring& s) {
		Parser p{};
		if (!p.load_string(s)) return Xml();
		return p.parse();
	}
}