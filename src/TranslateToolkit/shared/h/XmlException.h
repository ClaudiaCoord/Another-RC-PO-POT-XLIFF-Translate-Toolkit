/*
    Another RC/PO/POT/XLIFF Translate Toolkit.
    (c) CC 2023, MIT
    See README.md for more details.
*/

#pragma once

namespace xml {

	class XmlException {
	private:
		std::wstring err_{};
	public:

		XmlException(const std::wstring s) : err_(s) {}
		~XmlException() = default;

		const std::wstring what() const {
			return err_;
		}
	};

}
