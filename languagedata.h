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

#pragma once

#include <QMap>
#include <QMultiHash>
#include <QString>
#include <QVariantMap>

class QLatin1String;

struct ALanguage
{
    using wordDictionary = QMultiHash< QChar, QString >;

    ALanguage( const QString& Name, const QString& Def );

    void load();

    QString name;
    QString definitionName;
    int     id;
    bool    caseInsensitive;
    bool    loaded;

    QChar          comment;
    QChar          multilinestringchar;
    wordDictionary types;
    wordDictionary keywords;
    wordDictionary builtin;
    wordDictionary literals;
    wordDictionary others;

    private:
    void loadToDictionary(wordDictionary& D, QVariantList VL );
};

class LanguageDB
{
    public:
    LanguageDB();
    ~LanguageDB();

    ALanguage* language( const QString& Language );

    ALanguage* operator[]( const QString& Language );

    void load();

    public:
    QMap< QString, ALanguage* > Extensions;
    QMap< QString, ALanguage* > Languages;
    ALanguage*                  languageByExtension( const QString& Ext );
};
