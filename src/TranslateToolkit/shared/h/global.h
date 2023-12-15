/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <sstream>
#include <exception>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <array>
#include <tuple>
#include <memory>
#include <utility>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <future>
#include <atomic>
#include <chrono>
#include <format>
#include <set>

#include "argparse.h"
#include "configinfo.h"
#include "XmlException.h"

using namespace std::string_view_literals;
class config;

typedef std::pair<std::wstring, std::wstring> LANGP;
typedef std::vector<LANGP> LANGLIST;
typedef std::vector<std::wstring> EXCLUDELIST;
typedef std::set<std::wstring> HASHLIST;
typedef std::shared_ptr<config> CONFIG;

    class separators {
    public:
        static constexpr wchar_t command[] = L"& ";
        static constexpr wchar_t valid[] = L" \t";
        static constexpr wchar_t text[] = L"\"";
        static constexpr wchar_t end[] = L"\r\n";
        static constexpr wchar_t all[] = L" \t\v\f\r\n";
    };

    class console_writer {
    private:
        std::atomic<bool> is_write_{ false };
    public:

        void print(std::wstringstream ss) {
            while (is_write_.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            is_write_.store(true, std::memory_order_release);
            std::wcout << ss.str();
            std::wcout.flush();
            is_write_.store(false, std::memory_order_release);
        }
        void print(std::wstring s) {
            while (is_write_.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            is_write_.store(true, std::memory_order_release);
            std::wcout << s;
            std::wcout.flush();
            is_write_.store(false, std::memory_order_release);
        }
        void print(std::string s) {
            while (is_write_.load(std::memory_order_acquire))
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

            is_write_.store(true, std::memory_order_release);
            std::wcout << std::wstring(s.begin(), s.end());;
            std::wcout.flush();
            is_write_.store(false, std::memory_order_release);
        }

        template<typename T1>
        void print_trace(const wchar_t* c, std::string s, T1 t) {
            std::wstringstream ss{};
            ss << L"[" << std::wstring(c) << L"] " << std::wstring(s.begin(), s.end()) << L": ";
            if constexpr (std::is_same_v<std::wstring, T1>)
                ss << t;
            else if constexpr (std::is_same_v<std::string, T1>)
                ss << std::wstring(t.begin(), t.end());
            else
                ss << L"bad argument";
            ss << L"\n";
            print(std::move(ss));
        }
        void print_exception(std::exception_ptr ptr, std::wstring f) {
            if (!ptr) return;
            std::wstringstream ss{};

            try {
                try {
                    std::rethrow_exception(ptr);
                }
                catch (const xml::XmlException& e) {
                    ss << e.what();
                }
                catch (const std::future_error& e) {
                    ss << std::wstring(e.what(), e.what() + std::strlen(e.what()));
                }
                catch (const std::filesystem::filesystem_error& e) {
                    ss << std::wstring(e.what(), e.what() + std::strlen(e.what()));
                }
                catch (const std::runtime_error& e) {
                    ss << std::wstring(e.what(), e.what() + std::strlen(e.what()));
                }
                catch (const std::bad_exception& e) {
                    ss << std::wstring(e.what(), e.what() + std::strlen(e.what()));
                }
                catch (const std::exception& e) {
                    ss << std::wstring(e.what(), e.what() + std::strlen(e.what()));
                }
                catch (...) {
                    ss << L"Unknown exception";
                }
                std::wstring s = ss.str();
                if (!s.empty()) print(s);

            } catch (...) {}
        }
    } cw{};

    class utils {
    public:
        static std::wstring trim(std::wstring& s) {
            std::wstring w(s);
            static const wchar_t* t = L" \t\n\r\f\v";
            w.erase(0, w.find_first_not_of(t));
            w.erase(w.find_last_not_of(t) + 1U);
            return w;
        }
        static void build_exclude_list(EXCLUDELIST& v, std::filesystem::path& f) {
            try {
                std::filesystem::path p(f);
                if (p.empty() || !std::filesystem::exists(p)) {

                    wchar_t cpath[MAX_PATH + 1]{};
                    if (::GetModuleFileNameW(nullptr, cpath, MAX_PATH) != 0) {
                        std::filesystem::path p(cpath);
                        p.replace_extension(L".exclude");
                    }
                }
                if (std::filesystem::exists(p)) {

                    std::wifstream f(p.wstring(), std::ios::in | std::ios::binary);
                    if (f.is_open()) {
                        f.imbue(std::locale(".utf-8"));

                        std::wstring line{};
                        while (std::getline(f, line)) {
                            if (line.empty()) continue;
                            std::wstring s = utils::trim(line);
                            if (!s.empty()) v.push_back(s);
                        }
                    }
                }
            }
            catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
        }
        static const bool is_wdigit(std::wstring& s) {
            for (auto a : s)
                if (!::iswdigit(a)) return false;
            return true;
        }
        static std::wstring to_wstring(const char* c) {
            if (c == nullptr) return L"";
            return std::wstring(c, c + std::strlen(c));
        }
        static std::wstring to_wstring(std::string s) {
            if (s.empty()) return L"";
            return std::wstring(s.begin(), s.end());
        }
        static std::wstring po_header(std::wstring s, std::filesystem::path p) {
            std::wstringstream ss{};
            ss << L"#. extracted from " << p.filename().wstring().c_str() << L"\n"
                L"msgid \"\"\n"
                L"msgstr \"\"\n"
                L"\"Project-Id-Version: 9175\\n\"\n"
                L"\"Report-Msgid-Bugs-To: \\n\"\n"
                L"\"POT-Creation-Date: 2020-12-12 12:12+0000\\n\"\n"
                L"\"PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\\n\"\n"
                L"\"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n\"\n"
                L"\"Language-Team: LANGUAGE <LL@li.org>\\n\"\n"
                L"\"MIME-Version: 1.0\\n\"\n"
                L"\"Content-Type: text/plain; charset=UTF-8\\n\"\n"
                L"\"Content-Transfer-Encoding: 8bit\\n\"\n"
                L"\"X-Accelerator-Marker: &\\n\"\n"
                L"\"X-Generator: Translate " << s.c_str() << L" 1.0.0\\n\"\n"
                L"\"X-Merge-On: location\\n\"";
            return ss.str();
        }
        static std::wstring po_format(const std::wstring& s) {
            return po_format(s, L"");
        }
        static std::wstring po_format(const std::wstring& s, const std::wstring& b) {
            return (std::wstringstream() << L"\n\nmsgid \"" << s.c_str() << L"\"\nmsgstr \"" << b.c_str() << "\"").str();
        }
        static std::wstring& po_normalize(std::wstring& s) {
            try {
                if (s.length() < 2) return s;

                /* “ texts ” */
                size_t pos{ 0 };
                std::wstring& t(s);
                while (pos != std::wstring::npos) {
                    pos = t.find_first_of(separators::text, pos);
                    if (pos == std::wstring::npos) break;
                    t = t.replace(pos, 1, L"“");

                    pos = t.find_first_of(separators::text, (pos + 1));
                    if (pos == std::wstring::npos) break;
                    t = t.replace(pos++, 1, L"”");
                }
                /* "& texts */
                if (t.at(0) == separators::command[0] && t.starts_with(separators::command))
                    t = t.replace(0, 2, L"&");
                return t;
            }
            catch (...) {
                cw.print_exception(std::current_exception(), __FUNCTIONW__);
            }
            return s;
        }
    };
