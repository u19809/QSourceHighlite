/*
 * Copyright (c) 2019-2020 Waqar Ahmed -- <waqar.17a@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "qsourcehighlighter.h"
#include "languagedata.h"

#include <QDebug>
#include <QSettings>
#include <QTextDocument>
#include <algorithm>

QMap<QString, QSourceHighlighter::Theme> QSourceHighlighter::Themes;
static LanguageDB* AllLanguages = nullptr;

QSourceHighlighter::QSourceHighlighter( QTextDocument* doc )
    : QSyntaxHighlighter( doc )
    , _language( nullptr )
{
    if ( AllLanguages == nullptr )
    {

        Q_INIT_RESOURCE( qsourcehighlighterlanguages );

        AllLanguages = new LanguageDB();
        // fetch default language
        _language = AllLanguages->language( "c" );
    }

    {
        ALanguage* L = AllLanguages->language( "cpp" );
        CppID        = ( L ) ? L->id : -1;
    }
    {
        ALanguage* L = AllLanguages->language( "asm" );
        AsmID        = ( L ) ? L->id : -1;
    }
    {
        ALanguage* L = AllLanguages->language( "css" );
        CSSID        = ( L ) ? L->id : -1;
    }

    applyTheme("");
}

QSourceHighlighter::QSourceHighlighter(QTextDocument *doc, const QString &theme)
    : QSyntaxHighlighter(doc)
{
    applyTheme(theme);
}

bool QSourceHighlighter::setCurrentLanguage(const QString &language)
{
    if ( _language == nullptr || _language->name != language )
    {
        _language = AllLanguages->language( language );
    }

    return _language != nullptr;
}

bool QSourceHighlighter::setCurrentLanguageByExtension( const QString& ext )
{
    ALanguage* L = AllLanguages->languageByExtension( ext );
    if ( L )
    {
        if ( _language == nullptr || _language->name != L->name )
        {
            _language = L;
        }
    }

    return _language != nullptr;
}

QString QSourceHighlighter::currentLanguage()
{
    return ( _language != nullptr ) ? _language->name : "";
}

void QSourceHighlighter::highlightBlock( const QString& text )
{
    if ( _language == nullptr )
    {
        // no highlighting set
        return;
    }

    if ( currentBlock() == document()->firstBlock() )
    {
        setCurrentBlockState( _language->id );
    } else
    {
        setCurrentBlockState( previousBlockState() );
        //        previousBlockState() == _language->id ?
        //            setCurrentBlockState( _language->id ) :
        //            setCurrentBlockState( _language->id + 1 );
    }

    highlightSyntax( text );
}

/**
 * @brief Does the code syntax highlighting
 * @param text
 */
void QSourceHighlighter::highlightSyntax( const QString& text )
{
    if ( text.isEmpty() )
        return;

    if ( _language->name == "xml" )
    {
        xmlHighlighter( text );
        return;
    }

    const auto textLen = text.length();

    QChar comment = _language->comment;
    Qt::CaseSensitivity CI      = ( _language->caseInsensitive ) ? Qt::CaseInsensitive : Qt::CaseSensitive;
    bool  isCSS   = _language->name == "css";
    bool  isYAML  = _language->name == "yaml";
    bool  isMake  = _language->name == "make";
    bool  isAsm   = _language->name == "asm";

    // keep the default code block format
    // this statement is very slow
    // TODO: do this formatting when necessary instead of
    // applying it to the whole block in the beginning
    setFormat(0, textLen, _theme[CodeBlock]);

    QStringView ROText( text );

    auto applyCodeFormat =
        [ this, CI, ROText ]( int i, const ALanguage::wordDictionary& data,
                              const QString& text, const QTextCharFormat& fmt ) -> int {
        // check if we are at the beginning OR if this is the start of a word
        if ( i == 0 || ( !text.at( i - 1 ).isLetterOrNumber() && text.at( i - 1 ) != QChar( '_' ) ) )
        {

            const auto wordList = data.values( ( ( CI == Qt::CaseInsensitive ) ? ROText.at( i ).toLower() : ROText.at( i ) ) );
            for ( const QString& word : wordList )
            {
                // we have a word match check
                // 1. if we are at the end
                // 2. if we have a complete word
                if ( text.mid( i, word.size() ).compare( word, CI ) == 0 && ( i + word.size() == text.length() || ( !text.at( i + word.size() ).isLetterOrNumber() && text.at( i + word.size() ) != QChar( '_' ) ) ) )
                {
                    setFormat( i, word.size(), fmt );
                    i += word.size();
                }
            }
        }
        return i;
    };

    const QTextCharFormat &formatType = _theme[CodeType];
    const QTextCharFormat &formatKeyword = _theme[CodeKeyWord];
    const QTextCharFormat &formatComment = _theme[CodeComment];
    const QTextCharFormat &formatNumLit = _theme[CodeNumLiteral];
    const QTextCharFormat &formatBuiltIn = _theme[CodeBuiltIn];
    const QTextCharFormat &formatOther = _theme[CodeOther];

    for ( int i = 0; i < textLen; ++i )
    {

        if ( currentBlockState() % 3 == 1 )
        {
            goto Comment;
        }

        if ( currentBlockState() % 3 == 2 )
        {
            i = highlightStringLiterals( _language->multilinestringchar, text, i );
        }

        while ( i < textLen && !text[ i ].isLetter() )
        {
            if ( text[ i ].isSpace() )
            {
                ++i;
                //make sure we don't cross the bound
                if ( i == textLen )
                    return;
                if ( text[ i ].isLetter() )
                    break;
                else
                    continue;
            }
            //inline comment
            if ( comment.isNull() && text[ i ] == QChar( '/' ) )
            {
                if ( ( i + 1 ) < textLen )
                {
                    if ( text[ i + 1 ] == QChar( '/' ) )
                    {
                        setFormat( i, textLen, formatComment );
                        return;
                    } else if ( text[ i + 1 ] == QChar( '*' ) )
                    {
                    Comment:
                        int next = text.indexOf( QStringLiteral( "*/" ) );
                        if (next == -1) {
                            //we didn't find a comment end.
                            //Check if we are already in a comment block
                            if ( currentBlockState() % 3 == 1 )
                            {
                                setCurrentBlockState( _language->id + 1 );
                            }
                            setFormat(i, textLen,  formatComment);
                            return;
                        } else {
                            //we found a comment end
                            //mark this block as code if it was previously comment
                            //first check if the comment ended on the same line
                            //if modulo 2 is not equal to zero, it means we are in a comment
                            //-1 will set this block's state as language
                            if ( currentBlockState() % 3 == 0 )
                            {
                                setCurrentBlockState( _language->id );
                            }
                            next += 2;
                            setFormat(i, next - i,  formatComment);
                            i = next;
                            if (i >= textLen) return;
                        }
                    }
                }
            } else if ( text[ i ] == comment )
            {
                setFormat( i, textLen, formatComment );
                i = textLen;
                //integer lighteral
            } else if ( text[ i ].isNumber() )
            {
                i = highlightNumericLiterals( text, i );
                //string lighterals
            } else if ( text[ i ] == QChar( '\"' ) )
            {
                i = highlightStringLiterals( '\"', text, i );
            } else if ( text[ i ] == _language->multilinestringchar )
            {
                i = highlightStringLiterals( _language->multilinestringchar, text, i );
            } else if ( text[ i ] == QChar( '\'' ) )
            {
                i = highlightStringLiterals( '\'', text, i );
            }
            if (i >= textLen) {
                break;
            }
            ++i;
        }

        const int pos = i;

        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Types */
        i = applyCodeFormat( i, _language->types, text, formatType );
        /************************************************
         next letter is usually a space, in that case
         going forward is useless, so continue;
         We can ++i here and go to the beginning of the next word
         so that the next formatter can check for formatting but this will
         cause problems in case the next word is also of 'Type' or the current
         type(keyword/builtin). We can work around it and reset the value of i
         in the beginning of the loop to the word's first letter but I am not
         sure about its efficiency yet.
         ************************************************/
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Keywords */
        i = applyCodeFormat( i, _language->keywords, text, formatKeyword );
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Literals (true/false/NULL,nullptr) */
        i = applyCodeFormat(i, _language->literals, text, formatNumLit);
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight Builtin library stuff */
        i = applyCodeFormat( i, _language->builtin, text, formatBuiltIn );
        if (i == textLen || !text[i].isLetter()) continue;

        /* Highlight other stuff (preprocessor etc.) */
        if ( ( i == 0 || !text.at( i - 1 ).isLetter() ) && _language->others.contains( ( ( CI == Qt::CaseInsensitive ) ? ROText.at( i ).toLower() : ROText.at( i ) ) ) )
        {
            const QList< QString > wordList = _language->others.values( ( ( CI == Qt::CaseInsensitive ) ? ROText.at( i ).toLower() : ROText.at( i ) ) );
            for ( const QString& word : wordList )
            {
                if ( ROText.mid( i, word.size() ).compare( word ) == 0 // we have a word match
                     && ( i + word.size() == text.length()             // check if we are at the end
                          || !text.at( i + word.size() ).isLetter() )  //OR if we have a complete word
                )
                {
                    ( currentBlockState() == CppID ) ?
                        setFormat( i - 1, word.size() + 1, formatOther ) :
                        setFormat( i, word.size(), formatOther );
                    i += word.size();
                }
            }
        }

        //we were unable to find any match, lets skip this word
        if (pos == i) {
            int count = i;
            while (count < textLen) {
                if (!text[count].isLetter()) break;
                ++count;
            }
            i = count;
        }
    }

    if ( isCSS )
    {
        cssHighlighter( text );
    }
    if ( isYAML )
    {
        ymlHighlighter( text );
    }
    if ( isMake )
    {
        makeHighlighter( text );
    }
    if ( isAsm )
    {
        asmHighlighter( text );
    }
}

/**
 * @brief Highlight string lighterals in code
 * @param strType str type i.e., ' or "
 * @param text the text being scanned
 * @param i pos of i in loop
 * @return pos of i after the string
 */
int QSourceHighlighter::highlightStringLiterals(const QChar strType, const QString &text, int i) {
    bool stringClosed = false;
    setFormat(i, 1, _theme[CodeString]);
    ++i;

    while (i < text.length()) {
        //look for string end
        //make sure it's not an escape seq
        if ( text.at( i ) == strType && text.at( i - 1 ) != QChar( '\\' ) )
        {
            setFormat(i, 1, _theme[CodeString]);
            ++i;
            stringClosed = true;
            break;
        }
        //look for escape sequence
        if ( text.at( i ) == QChar( '\\' ) && ( i + 1 ) < text.length() )
        {
            int len = 0;
            switch ( text.at( i + 1 ).toLatin1() )
            {
                case 'a':
                case 'b':
                case 'e':
                case 'f':
                case 'n':
                case 'r':
                case 't':
                case 'v':
                case '\'':
                case '"':
                case '\\':
                case '\?':
                    //2 because we have to highlight \ as well as the following char
                    len = 2;
                    break;
                //octal esc sequence \123
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': {
                    if ( i + 4 <= text.length() )
                    {
                        if ( !isOctal( text.at( i + 2 ) ) )
                        {
                            break;
                        }
                        if ( !isOctal( text.at( i + 3 ) ) )
                        {
                            break;
                        }
                        len = 4;
                    }
                    break;
                }
                //hex numbers \xFA
                case 'x': {
                    if ( i + 3 <= text.length() )
                    {
                        if ( !isHex( text.at( i + 2 ) ) )
                        {
                            break;
                        }
                        if ( !isHex( text.at( i + 3 ) ) )
                        {
                            break;
                        }
                        len = 4;
                    }
                    break;
                }
                //TODO: implement unicode code point escaping
                default:
                    break;
            }

            //if len is zero, that means this wasn't an esc seq
            //increment i so that we skip this backslash
            if (len == 0) {
                    setFormat(i, 1, _theme[CodeString]);
                    ++i;
                    continue;
            }

            setFormat(i, len, _theme[CodeNumLiteral]);
            i += len;
            continue;
        }
        setFormat(i, 1, _theme[CodeString]);
        ++i;
    }

    setCurrentBlockState( _language->id + ( ( !stringClosed ) ? 2 : 0 ) );
    return i;
}

/**
 * @brief Highlight number lighterals in code
 * @param text the text being scanned
 * @param i pos of i in loop
 * @return pos of i after the number
 */
int QSourceHighlighter::highlightNumericLiterals(const QString &text, int i)
{
    bool isPreAllowed = false;
    if (i == 0) isPreAllowed = true;
    else {
        //these values are allowed before a number
        switch ( text.at( i - 1 ).toLatin1() )
        {
            //css number
            case ':':
                if ( currentBlockState() == CSSID )
                    isPreAllowed = true;
                break;
            case '$':
                if ( currentBlockState() == AsmID )
                    isPreAllowed = true;
                break;
            case '[':
            case '(':
            case '{':
            case ' ':
            case ',':
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '<':
            case '>':
                isPreAllowed = true;
                break;
        }
    }

    if (!isPreAllowed) return i;

    const int start = i;

    if ((i+1) >= text.length()) {
        setFormat(i, 1, _theme[CodeNumLiteral]);
        return ++i;
    }

    ++i;
    //hex numbers highlighting (only if there's a preceding zero)
    if (text.at(i) == QChar('x') && text.at(i - 1) == QChar('0'))
        ++i;

    while (i < text.length()) {
        if (!text.at(i).isNumber() && text.at(i) != QChar('.') &&
             text.at(i) != QChar('e')) //exponent
            break;
        ++i;
    }

    bool isPostAllowed = false;
    if (i == text.length()) {
        //cant have e at the end
        if (text.at(i - 1) != QChar('e'))
            isPostAllowed = true;
    } else {
        //these values are allowed after a number
        switch ( text.at( i ).toLatin1() )
        {
            case ']':
            case ')':
            case '}':
            case ' ':
            case ',':
            case '=':
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '>':
            case '<':
            case ';':
                isPostAllowed = true;
                break;
            // for 100u, 1.0F
            case 'p':
                if ( currentBlockState() == CSSID )
                    if ( i + 1 < text.length() && text.at( i + 1 ) == QChar( 'x' ) )
                    {
                        if ( i + 2 == text.length() || !text.at( i + 2 ).isLetterOrNumber() )
                            isPostAllowed = true;
                    }
                break;
            case 'e':
                if ( currentBlockState() == CSSID )
                    if ( i + 1 < text.length() && text.at( i + 1 ) == QChar( 'm' ) )
                    {
                        if ( i + 2 == text.length() || !text.at( i + 2 ).isLetterOrNumber() )
                            isPostAllowed = true;
                    }
                break;
            case 'u':
            case 'l':
            case 'f':
            case 'U':
            case 'L':
            case 'F':
                if ( i + 1 == text.length() || !text.at( i + 1 ).isLetterOrNumber() )
                {
                    isPostAllowed = true;
                    ++i;
                }
                break;
        }
    }
    if (isPostAllowed) {
        int end = i;
        setFormat(start, end - start, _theme[CodeNumLiteral]);
    }
    //decrement so that the index is at the last number, not after it
    return --i;
}

/**
 * @brief The YAML highlighter
 * @param text
 * @details This function post processes a line after the main syntax
 * highlighter has run for additional highlighting. It does these things
 *
 * If the current line is a comment, skip it
 *
 * Highlight all the words that have a colon after them as 'keyword' except:
 * If the word is a string, skip it.
 * If the colon is in between a path, skip it (C:\)
 *
 * Once the colon is found, the function will skip every character except 'h'
 *
 * If an h letter is found, check the next 4/5 letters for http/https and
 * highlight them as a link (underlined)
 */
void QSourceHighlighter::ymlHighlighter(TextValue text)
{
    if (text.isEmpty())
        return;
    const auto textLen = text.length();
    bool colonNotFound = false;

    //if this is a comment don't do anything and just return
    if ( text.trimmed().at( 0 ) == QChar( '#' ) )
        return;

    for (int i = 0; i < textLen; ++i) {
        if (!text.at(i).isLetter()) continue;

        if ( colonNotFound && text.at( i ) != QChar( 'h' ) )
            continue;

        //we found a string lighteral, skip it
        if ( i != 0 && ( text.at( i - 1 ) == QChar( '"' ) || text.at( i - 1 ) == QChar( '\'' ) ) )
        {
            const int next = text.indexOf(text.at(i-1), i);
            if (next == -1) break;
            i = next;
            continue;
        }

        const int colon = text.indexOf( QChar( ':' ), i );

        //if colon isn't found, we set this true
        if (colon == -1) colonNotFound = true;

        if (!colonNotFound) {
            //if the line ends here, format and return
            if (colon + 1 == textLen) {
                    setFormat(i, colon - i, _theme[CodeKeyWord]);
                    return;
            } else {
                    //colon is found, check if it isn't some path or something else
                    if (!(text.at(colon + 1) == QChar('\\') && text.at(colon + 1) == QChar('/'))) {
                        setFormat(i, colon - i, _theme[CodeKeyWord]);
                    }
            }
        }

        //underlined links
        if ( text.at( i ) == QChar( 'h' ) )
        {
            if (text.MIDREF(i, 5) == QStringLiteral("https")
                || text.MIDREF(i, 4) == QStringLiteral("http")) {
                int space = text.indexOf(QChar(' '), i);
                if (space == -1) space = textLen;
                QTextCharFormat f = _theme[CodeString];
                f.setUnderlineStyle(QTextCharFormat::SingleUnderline);
                setFormat(i, space - i, f);
                i = space;
            }
        }
    }
}

void QSourceHighlighter::cssHighlighter(TextValue text)
{
    if (text.isEmpty()) return;
    const auto textLen = text.length();
    for (int i = 0; i<textLen; ++i) {
        if ( text[ i ] == QChar( '.' ) || text[ i ] == QChar( '#' ) )
        {
            if (i+1 >= textLen) return;
            if (text[i + 1].isSpace() || text[i+1].isNumber()) continue;
            int space = text.indexOf( QChar( ' ' ), i );
            if (space < 0) {
                space = text.indexOf('{');
                if (space < 0) {
                    space = textLen;
                }
            }
            setFormat(i, space - i, _theme[CodeKeyWord]);
            i = space;
        } else if ( text[ i ] == QChar( 'c' ) )
        {
            if (text.mid(i, 5) == QStringLiteral("color")) {
                i += 5;
                int colon = text.indexOf( QChar( ':' ), i );
                if (colon < 0) continue;
                i = colon;
                i++;
                while(i < textLen) {
                    if (!text[i].isSpace()) break;
                    i++;
                }
                int semicolon = text.indexOf( QChar( ';' ) );
                if (semicolon < 0)
                    semicolon = textLen;
                TextBuffer color = text.mid(i, semicolon - i);
                QTextCharFormat f = _theme[CodeBlock];
                QColor c(color);
                if ( color.startsWith( QStringLiteral( "rgb" ) ) )
                {
                    int t    = text.indexOf( QChar( '(' ), i );
                    int rPos = text.indexOf( QChar( ',' ), t );
                    int gPos = text.indexOf( QChar( ',' ), rPos + 1 );
                    int bPos = text.indexOf( QChar( ')' ), gPos );
                    if (rPos > -1 && gPos > -1 && bPos > -1) {
                            TextRef r = text.MIDREF(t + 1, rPos - (t + 1));
                            TextRef g = text.MIDREF(rPos + 1, gPos - (rPos + 1));
                            TextRef b = text.MIDREF(gPos + 1, bPos - (gPos + 1));
                            c.setRgb(r.toInt(), g.toInt(), b.toInt());
                    } else {
                            c = _theme[CodeBlock].background().color();
                    }
                }

                if (!c.isValid()) {
                    continue;
                }

                int lightness{};
                QColor foreground;
                //really dark
                if (c.lightness() <= 20) {
                    foreground = Qt::white;
                } else if (c.lightness() > 20 && c.lightness() <= 51){
                    foreground = QColor( 12, 12, 12 );
                } else if (c.lightness() > 51 && c.lightness() <= 78){
                    foreground = QColor( 11, 11, 11 );
                } else if (c.lightness() > 78 && c.lightness() <= 110){
                    foreground = QColor( 10, 10, 10 );
                } else if (c.lightness() > 127) {
                    lightness = c.lightness() + 100;
                    foreground = c.darker(lightness);
                }
                else {
                    lightness = c.lightness() + 100;
                    foreground = c.lighter(lightness);
                }

                f.setBackground(c);
                f.setForeground(foreground);
                setFormat(i, semicolon - i, QTextCharFormat()); //clear prev format
                setFormat(i, semicolon - i, f);
                i = semicolon;
            }
        }
    }
}

void QSourceHighlighter::xmlHighlighter(TextValue text)
{
    if (text.isEmpty()) return;
    const auto textLen = text.length();

    setFormat(0, textLen, _theme[CodeBlock]);

    for (int i = 0; i < textLen; ++i) {
        if ( text[ i ] == QChar( '<' ) && text[ i + 1 ] != QChar( '!' ) )
        {

            const int found = text.indexOf( QChar( '>' ), i );
            if (found > 0) {
                ++i;
                if ( text[ i ] == QChar( '/' ) )
                    ++i;
                setFormat(i, found - i, _theme[CodeKeyWord]);
            }
        }

        if ( text[ i ] == QChar( '=' ) )
        {
            int lastSpace = text.lastIndexOf( QChar( ' ' ), i );
            if ( lastSpace == i - 1 )
                lastSpace = text.lastIndexOf( QChar( ' ' ), i - 2 );
            if (lastSpace > 0) {
                setFormat(lastSpace, i - lastSpace, _theme[CodeBuiltIn]);
            }
        }

        if ( text[ i ] == QChar( '\"' ) )
        {
            const int pos = i;
            int cnt = 1;
            ++i;
            //bound check
            if ( (i+1) >= textLen) return;
            while (i < textLen) {
                if ( text[ i ] == QChar( '\"' ) )
                {
                    ++cnt;
                    ++i;
                    break;
                }
                ++i; ++cnt;
                //bound check
                if ( (i+1) >= textLen) {
                    ++cnt;
                    break;
                }
            }
            setFormat(pos, cnt, _theme[CodeString]);
        }
    }
}

void QSourceHighlighter::makeHighlighter(TextValue text)
{
    int colonPos = text.indexOf( QChar( ':' ) );
    if (colonPos == -1)
        return;
    setFormat(0, colonPos, _theme[Token::CodeBuiltIn]);
}

/**
 * @brief highlight inline labels such as 'func()' in "call func()"
 * @param text
 */
void QSourceHighlighter::highlightInlineAsmLabels(TextValue text)
{
#define Q(s) QStringLiteral(s)
    static const QString jumps[27] = {
        //0 - 19
        Q("jmp"), Q("je"), Q("jne"), Q("jz"), Q("jnz"), Q("ja"), Q("jb"), Q("jg"), Q("jge"), Q("jae"), Q("jl"), Q("jle"),
        Q("jbe"), Q("jo"), Q("jno"), Q("js"), Q("jns"), Q("jcxz"), Q("jecxz"), Q("jrcxz"),
        //20 - 24
        Q("loop"), Q("loope"), Q("loopne"), Q("loopz"), Q("loopnz"),
        //25 - 26
        Q("call"), Q("callq")
    };
#undef Q

    auto format = _theme[Token::CodeBuiltIn];
    format.setFontUnderline(true);

    TextBuffer trimmed = text.trimmed();
    int start = -1;
    int end = -1;
    QChar         c;
    if (!trimmed.isEmpty())
        c = trimmed.at( 0 );
    if (c == 'j') {
        start = 0; end = 20;
    } else if (c == 'c') {
        start = 25; end = 27;
    } else if (c == 'l') {
        start = 20; end = 25;
    } else {
        return;
    }

    auto skipSpaces = [&text](int& j){
        while (text.at(j).isSpace()) j++;
        return j;
    };

    for (int i = start; i < end; ++i) {
        if (trimmed.startsWith(jumps[i])) {
            int j = 0;
            skipSpaces(j);
            j = j + jumps[i].length() + 1;
            skipSpaces(j);
            int len = text.length() - j;
            setFormat(j, len, format);
        }
    }
}

void QSourceHighlighter::asmHighlighter(TextValue text)
{
    highlightInlineAsmLabels(text);
    //label highlighting
    //examples:
    //L1:
    //LFB1:           # local func begin
    //
    //following e.gs are not a label
    //mov %eax, Count::count(%rip)
    //.string ": #%s"

    //look for the last occurence of a colon
    int colonPos = text.lastIndexOf( QChar( ':' ) );
    if (colonPos == -1)
        return;
    //check if this colon is in a comment maybe?
    bool isComment = text.lastIndexOf('#', colonPos) != -1;
    if (isComment) {
        int commentPos = text.lastIndexOf('#', colonPos);
        colonPos = text.lastIndexOf(':', commentPos);
    }

    auto format = _theme[Token::CodeBuiltIn];
    format.setFontUnderline(true);

    if (colonPos >= text.length() - 1) {
        setFormat(0, colonPos, format);
    }

    int i = 0;
    bool isLabel = true;
    for (i = colonPos + 1; i < text.length(); ++i) {
        if (!text.at(i).isSpace()) {
            isLabel = false;
            break;
        }
    }

    if ( !isLabel && i < text.length() && text.at( i ) == QChar( '#' ) )
        setFormat(0, colonPos, format);
}

#if Q_VERSION_MAJOR >= 6
void QSourceHighlighter::applyTheme(QStringView themeName)
#else
void QSourceHighlighter::applyTheme(const QString &themeName)
#endif
{
    _theme = getTheme(themeName);
    rehighlight();
}

#if Q_VERSION_MAJOR >= 6
void QSourceHighlighter::addTheme(QStringView themeName, const Theme &theme)
#else
void QSourceHighlighter::addTheme(const QString &themeName, const Theme &theme)
#endif
{
    Themes[themeName] = theme;
}

#if Q_VERSION_MAJOR >= 6
QSourceHighlighter::Theme QSourceHighlighter::getTheme(QStringView themeName)
#else
QSourceHighlighter::Theme QSourceHighlighter::getTheme(const QString &themeName)
#endif
{
    if (Themes.isEmpty()) {
        {
            Theme T;
            T[QSourceHighlighter::Token::CodeBlock] = QTextCharFormat();
            T[QSourceHighlighter::Token::CodeKeyWord].setForeground(QColor(0xF92672));
            T[QSourceHighlighter::Token::CodeString].setForeground(QColor(0xa39b4e));
            T[QSourceHighlighter::Token::CodeComment].setForeground(QColor(0x75715E));
            T[QSourceHighlighter::Token::CodeType].setForeground(QColor(0x54aebf));
            T[QSourceHighlighter::Token::CodeOther].setForeground(QColor(0xdb8744));
            T[QSourceHighlighter::Token::CodeNumLiteral].setForeground(QColor(0xAE81FF));
            T[QSourceHighlighter::Token::CodeBuiltIn].setForeground(QColor(0x018a0f));
            addTheme("", T);
        }

        {
            Theme T;
            T[QSourceHighlighter::Token::CodeBlock].setForeground(QColor(227, 226, 214));
            T[QSourceHighlighter::Token::CodeKeyWord].setForeground(QColor(249, 38, 114));
            T[QSourceHighlighter::Token::CodeString].setForeground(QColor(230, 219, 116));
            T[QSourceHighlighter::Token::CodeComment].setForeground(QColor(117, 113, 94));
            T[QSourceHighlighter::Token::CodeType].setForeground(QColor(102, 217, 239));
            T[QSourceHighlighter::Token::CodeOther].setForeground(QColor(249, 38, 114));
            T[QSourceHighlighter::Token::CodeNumLiteral].setForeground(QColor(174, 129, 255));
            T[QSourceHighlighter::Token::CodeBuiltIn].setForeground(QColor(166, 226, 46));
            addTheme("monokai", T);
        }
    }

    Theme T = Themes.value(themeName);
    return (T.isEmpty()) ? getTheme("") : T;
}

#if Q_VERSION_MAJOR >= 6
QSourceHighlighter::Theme::Theme(QSettings *S, QStringView Key)
#else
QSourceHighlighter::Theme::Theme(QSettings *S, const QString &Key)
#endif
{
    S->beginGroup(Key);
    QMapIterator<QString, Token> MI(colors());
    while (MI.hasNext()) {
        MI.next();
        (*this)[MI.value()].setForeground(S->value(MI.key()).value<QColor>());
    }
    S->endGroup();
}

const QMap<QString, QSourceHighlighter::Token> &QSourceHighlighter::Theme::colors()
{
    static QMap<QString, QSourceHighlighter::Token> C{{"block", QSourceHighlighter::CodeBlock},
                                                    {"keyword", QSourceHighlighter::CodeKeyWord},
                                                    {"string", QSourceHighlighter::CodeString},
                                                    {"comment", QSourceHighlighter::CodeComment},
                                                    {"type", QSourceHighlighter::CodeType},
                                                    {"other", QSourceHighlighter::CodeOther},
                                                    {"numlighteral", QSourceHighlighter::CodeNumLiteral},
                                                    {"builtin", QSourceHighlighter::CodeBuiltIn}};
    return C;
}
