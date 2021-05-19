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
#include <QDebug>

#include "languagedata.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMultiHash>
#include <QString>

ALanguage::ALanguage( const QString& Name, const QString& Def )
    : comment( '/' )
{
    static int idGen = 0;
    name             = Name;
    caseInsensitive  = false;
    definitionName   = Def;
    id               = idGen;
    idGen += 3; // 1 for comment and 1 for multiline strings
    loaded = false;
}

void ALanguage::loadToDictionary( ALanguage::wordDictionary& D, QVariantList VL )
{
    foreach ( auto V, VL )
    {
        D.insert( V.toString()[ 0 ].toLatin1(), V.toString() );
    }
}

void ALanguage::load()
{
    QFile F( QStringLiteral( ":/languages/language_%1.json" ).arg( definitionName ) );

    if ( !F.open( QIODevice::ReadOnly ) )
    {
        qWarning() << "Error loading sourcehighliter syntax file" << F.fileName() << F.errorString();
        return;
    }

    QJsonParseError Err;
    QByteArray      FF = F.readAll();
    QVariant        V  = QJsonDocument::fromJson( FF, &Err ).toVariant();

    if ( Err.error != QJsonParseError::NoError )
    {
        qWarning() << "Error loading sourcehighliter syntax file" << F.fileName() << Err.errorString() << "near" << QString::fromUtf8( FF ).mid( qMax( 0, Err.offset - 10 ), 30 );
        return;
    }

    // load data
    QVariantMap Defs = V.toMap();
    if ( Defs.contains( "comment" ) )
    {
        comment = Defs.value( "comment" ).toString().at( 0 );
    }
    if ( Defs.contains( "multilinestringchar" ) )
    {
        multilinestringchar = Defs.value( "multilinestringchar" ).toString().at( 0 );
    }
    caseInsensitive = Defs.value( "caseInsensitive", false ).toBool();

    loadToDictionary( types, Defs.value( "types" ).toList() );
    loadToDictionary( keywords, Defs.value( "keywords" ).toList() );
    loadToDictionary( builtin, Defs.value( "builtin" ).toList() );
    loadToDictionary( literals, Defs.value( "literals" ).toList() );
    loadToDictionary( others, Defs.value( "others" ).toList() );
}

LanguageDB::LanguageDB()
{
    QFile F( ":/languages/languages.json" );

    Q_ASSERT( F.open( QIODevice::ReadOnly ) );

    QJsonParseError Err;
    QByteArray      FF = F.readAll();
    QVariant        V  = QJsonDocument::fromJson( FF, &Err ).toVariant();

    if ( Err.error != QJsonParseError::NoError )
    {
        qCritical() << "Error loading sourcehighliter file" << F.fileName() << Err.errorString() << "near" << QString::fromUtf8( FF ).mid( qMax( 0, Err.offset - 10 ), 30 );
        Q_ASSERT( false );
    }

    QMapIterator< QString, QVariant > MI( V.toMap() );
    while ( MI.hasNext() )
    {
        MI.next();
        Languages[ MI.key() ] = new ALanguage( MI.key(), MI.value().toMap()[ "name" ].toString() );
        foreach ( auto V, MI.value().toMap()[ "extensions" ].toStringList() )
        {
            Extensions[ V ] = Languages[ MI.key() ];
        }
    }

    // add builtin language XML
    Languages[ "xml" ]  = new ALanguage( "xml", "xml" );
    Extensions[ "xml" ] = Languages[ "xml" ];
    Languages[ "xml" ]->loaded = true;
}

LanguageDB::~LanguageDB()
{ }

ALanguage* LanguageDB::language( const QString& Language )
{
    ALanguage* L = Languages.value( Language );
    if ( L == nullptr )
    {
        qWarning() << "No syntax file for language" << Language;
        return L;
    }

    if ( !L->loaded )
    {
        L->load();
    }

    return L;
}

ALanguage* LanguageDB::languageByExtension( const QString& Ext )
{
    ALanguage* L = Extensions.value( Ext );
    if ( L == nullptr )
    {
        qWarning() << "No syntax file for extension" << Ext;
        return L;
    }
    if ( !L->loaded )
    {
        L->load();
    }

    return L;
}

ALanguage* LanguageDB::operator[]( const QString& Language )
{
    return language( Language );
}
