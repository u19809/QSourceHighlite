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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qsourcehighlighter.h"

#include <QDebug>
#include <QDir>

QHash<QString, QSourceHighlighter::Language> MainWindow::_langStringToEnum;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    initLangsEnum();
    initLangsComboBox();
    initThemesComboBox();

    //set highlighter
    QFont f = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    ui->plainTextEdit->setFont(f);
    highlighter = new QSourceHighlighter(ui->plainTextEdit->document());

    connect(ui->langComboBox,
            static_cast<void (QComboBox::*) (const QString&)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::languageChanged);
    connect(ui->themeComboBox,
            static_cast<void (QComboBox::*) (int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::themeChanged);

    ui->langComboBox->setCurrentText("Asm");
    languageChanged("Asm");
    //    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, &MainWindow::printDebug);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initLangsEnum()
{
    MainWindow::_langStringToEnum = QHash<QString, QSourceHighlighter::Language> {
        { QLatin1String("Asm"), QSourceHighlighter::CodeAsm },
        { QLatin1String("Bash"), QSourceHighlighter::CodeBash },
        { QLatin1String("C"), QSourceHighlighter::CodeC },
        { QLatin1String("C++"), QSourceHighlighter::CodeCpp },
        { QLatin1String("CMake"), QSourceHighlighter::CodeCMake },
        { QLatin1String("CSharp"), QSourceHighlighter::CodeCSharp },
        { QLatin1String("Css"), QSourceHighlighter::CodeCSS },
        { QLatin1String("Go"), QSourceHighlighter::CodeGo },
        { QLatin1String("Html"), QSourceHighlighter::CodeXML },
        { QLatin1String("Ini"), QSourceHighlighter::CodeINI },
        { QLatin1String("Java"), QSourceHighlighter::CodeJava },
        { QLatin1String("Javascript"), QSourceHighlighter::CodeJava },
        { QLatin1String("Json"), QSourceHighlighter::CodeJSON },
        { QLatin1String("Lua"), QSourceHighlighter::CodeLua },
        { QLatin1String("Make"), QSourceHighlighter::CodeMake },
        { QLatin1String("Php"), QSourceHighlighter::CodePHP },
        { QLatin1String("Python"), QSourceHighlighter::CodePython },
        { QLatin1String("Qml"), QSourceHighlighter::CodeQML },
        { QLatin1String("Rust"), QSourceHighlighter::CodeRust },
        { QLatin1String("Sql"), QSourceHighlighter::CodeSQL },
        { QLatin1String("Typescript"), QSourceHighlighter::CodeTypeScript },
        { QLatin1String("V"), QSourceHighlighter::CodeV },
        { QLatin1String("Vex"), QSourceHighlighter::CodeVex },
        { QLatin1String("Xml"), QSourceHighlighter::CodeXML },
        { QLatin1String("Yaml"), QSourceHighlighter::CodeYAML }
    };
}

void MainWindow::initThemesComboBox()
{
    ui->themeComboBox->addItem("Monokai", QSourceHighlighter::Themes::Monokai);
    ui->themeComboBox->addItem("debug", QSourceHighlighter::Themes::Monokai);
}

void MainWindow::initLangsComboBox() {
    ui->langComboBox->addItem("Asm");
    ui->langComboBox->addItem("Bash");
    ui->langComboBox->addItem("C");
    ui->langComboBox->addItem("C++");
    ui->langComboBox->addItem("CMake");
    ui->langComboBox->addItem("CSharp");
    ui->langComboBox->addItem("Css");
    ui->langComboBox->addItem("Go");
    ui->langComboBox->addItem("Html");
    ui->langComboBox->addItem("Ini");
    ui->langComboBox->addItem("Javascript");
    ui->langComboBox->addItem("Java");
    ui->langComboBox->addItem("Lua");
    ui->langComboBox->addItem("Make");
    ui->langComboBox->addItem("Php");
    ui->langComboBox->addItem("Python");
    ui->langComboBox->addItem("Qml");
    ui->langComboBox->addItem("Rust");
    ui->langComboBox->addItem("Sql");
    ui->langComboBox->addItem("Typescript");
    ui->langComboBox->addItem("V");
    ui->langComboBox->addItem("Vex");
    ui->langComboBox->addItem("Xml");
    ui->langComboBox->addItem("Yaml");
}

void MainWindow::themeChanged(int) {
    QSourceHighlighter::Themes theme = (QSourceHighlighter::Themes)ui->themeComboBox->currentData().toInt();
    highlighter->setTheme(theme);
}

void MainWindow::languageChanged(const QString &lang) {
    highlighter->setCurrentLanguage(_langStringToEnum.value(lang));

    QFile f(QDir::currentPath() + "/../test_files/" + lang + ".txt");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto text = f.readAll();
        ui->plainTextEdit->setPlainText(QString::fromUtf8(text));
    }
    f.close();
}
