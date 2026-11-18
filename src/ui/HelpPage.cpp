#include "ui/HelpPage.h"
#include "ui/Theme.h"

#include "ui/Controls.h"

#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtGlobal>

namespace toupie {

HelpPage::HelpPage(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    auto *title = Theme::createTitle(tr("Aide — commandes"), this);
    root->addWidget(title);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    m_text = new QLabel(scroll);
    m_text->setTextFormat(Qt::RichText);
    m_text->setWordWrap(true);
    m_text->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scroll->setWidget(m_text);
    root->addWidget(scroll, 1);

    auto *buttons = new QHBoxLayout;
    auto *back = new QPushButton(tr("Retour au menu"), this);
    back->setProperty("variant", QStringLiteral("ghost"));
    back->setProperty("glyph", QStringLiteral("back"));
    auto *settings = new QPushButton(tr("Modifier les touches…"), this);
    auto *licenses = new QPushButton(tr("Licences"), this);
    licenses->setProperty("variant", QStringLiteral("ghost"));
    buttons->addWidget(back);
    buttons->addStretch();
    buttons->addWidget(licenses);
    buttons->addWidget(settings);
    root->addLayout(buttons);
    connect(back, &QPushButton::clicked, this, &HelpPage::backRequested);
    connect(settings, &QPushButton::clicked, this, &HelpPage::settingsRequested);
    connect(licenses, &QPushButton::clicked, this, &HelpPage::showLicenses);
    refresh();
}

QString HelpPage::licensesText()
{
    auto read = [](const QString &path) {
        QFile file(path);
        return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
    };
    QString text = tr("Top Arena %1 · Qt %2 (bibliothèques Qt embarquées)")
                       .arg(QCoreApplication::applicationVersion(), QString::fromLatin1(qVersion()))
                   + QStringLiteral("\n\n") + read(QStringLiteral(":/LICENSE")) + QStringLiteral("\n\n")
                   + read(QStringLiteral(":/THIRD-PARTY-NOTICES.md"));
    for (const char *path : {":/licenses/LGPL-3.0.txt", ":/licenses/GPL-3.0.txt", ":/licenses/LGPL-2.1.txt",
                             ":/fonts/OFL-ChakraPetch.txt", ":/fonts/OFL-Rajdhani.txt"}) {
        const QString name = QString::fromLatin1(path).section(QLatin1Char('/'), -1);
        text += QStringLiteral("\n\n==== %1 ====\n\n").arg(name) + read(QString::fromLatin1(path));
    }
    return text;
}

void HelpPage::showLicenses()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Licences"));
    dialog.resize(820, 640);
    auto *layout = new QVBoxLayout(&dialog);
    auto *view = new QPlainTextEdit(licensesText(), &dialog);
    view->setReadOnly(true);
    layout->addWidget(view);
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(close);
    dialog.exec();
}

void HelpPage::refresh()
{
    m_text->setText(Controls::instance().helpHtml());
}

} // namespace toupie
