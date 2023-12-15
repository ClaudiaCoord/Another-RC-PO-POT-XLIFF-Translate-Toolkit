# Another RC/PO/POT/XLIFF Translate Toolkit

The reason why these utilities were written is simple; I could not find working analogues.
Of course, there is the `Translate Toolkit` from © `Translate House`, but from version to version its performance is steadily decreasing...

<details>
We used original "Translate Toolkit" package before. In addition to the package itself, for full functionality you must also install the latest version of "GetText" from "GNU". From it you need the merge utility, which tidies up the multi-line output from the 'rc2po' and 'po2rc' utilities. This format complies with the standards, but many online resources related to translation into different languages cannot work with it correctly. This is especially evident in the Chinese or Japanese languages: lines are cut off, syntax format errors appear related to unclosed quotes and many other similar faults.
</details>
  
The main disadvantages that prevent you from using the original Translate Toolkit:

<details>
"po2rc" utility:
  
1. Does not understand menu tags unless they are inside the 'POPUP' tag.
2. Dialogue titles, tags 'STYLE', 'FONT', 'CAPTION', 'MENU' are written in one long line, after which the assembly of the RC file causes an error.
3. Does not understand constructions like '#, fussy', produces the error: "error line:1 symbol:2", regardless of location. At the same time, other utilities from the same package generate just such constructs, for example 'xliff2po'.
4. If the 'PO' file ends with an empty line, it also produces a similar error that has nothing to do with the problem.
5. If the source PO file is in a format other than UTF-8, multiple errors are possible, the origin of which is not clear.
6. If you specify to use UTF-8 encoding for the output file, the file will still be written as UNICODE, in UTF-16 LE format. The way out of this situation is to subsequently convert the output file into UTF-8 format using third-party programs.

"rc2po" utility:

1. Does not work correctly with escaped quotes in text, leaves unclosed lines, the file is corrupted.
2. It does not always process constructs like '{0}/{1}' related to the string format correctly; the file is corrupted.
3. It does not filter by numeric values, that is, strings consisting only of numbers will also be added to the translation.
4. Adds empty lines consisting of one space to the translation.
4. It does not have settings that affect multi-line output of values; it is impossible to change this behavior.
5. Does not have settings to prevent spam recording of line identifiers, thereby increasing the file size several times. This makes viewing and analyzing the source file very difficult.
6. When using UTF-8 and missing the 'BOM' header at the beginning of the file, it produces the error: "error line:1 symbol:2".

"xliff2po" utility:

1. Adds the construction '#, fussy' to each 'msgid + msgtext' pair; other utilities from the same package do not understand this construction, which leads to a processing error. There is no way to disable this behavior.
2. Does not replace the '&' sign in the 'xliff' format with the '&amp;' html tag. Since the 'xliff' format is a subset of the 'XML' format, this results in an error. No further processing of such a file is possible.
</details>

Also, we must take into account that these utilities in the original Translate Toolkit package are written in `python`. This affects processing speed. With a large volume of files, more than 100, the conversion time becomes noticeable.

All utilities included in the `Another Translate Toolkit` package are written in C++, so the processing speed of language files is close to maximum. Compiled packages are available for Windows x64 and x86 platforms.

