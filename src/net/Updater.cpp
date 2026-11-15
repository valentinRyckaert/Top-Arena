#include "net/Updater.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QProcess>
#include <QStandardPaths>
#include <QVersionNumber>

#include <filesystem>
#include <memory>

namespace toupie::net {

namespace {
const QUrl kLatestRelease(
    QStringLiteral("https://api.github.com/repos/valentinRyckaert/Top-Arena/releases/latest"));

QVersionNumber parseVersion(QString text)
{
    if (text.startsWith(QLatin1Char('v')))
        text.remove(0, 1);
    return QVersionNumber::fromString(text);
}

QNetworkRequest request(const QUrl &url)
{
    QNetworkRequest r(url);
    r.setHeader(QNetworkRequest::UserAgentHeader,
                QStringLiteral("ToupieArena/%1").arg(QCoreApplication::applicationVersion()));
    return r;
}

#ifdef Q_OS_MACOS
/// Bundle .app contenant l'exécutable, vide sinon.
QString appBundle()
{
    const QString bundle = QDir(QCoreApplication::applicationDirPath() + QStringLiteral("/../.."))
                               .canonicalPath();
    return bundle.endsWith(QStringLiteral(".app")) ? bundle : QString();
}
#endif
/// Démarre un programme détaché. Environnement propre : celui d'une AppImage
/// pointe vers son point de montage, qui disparaît à sa fermeture.
bool launch(const QString &program, const QStringList &arguments)
{
    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains(QStringLiteral("APPIMAGE"))) {
        for (const char *name : {"APPIMAGE", "APPDIR", "ARGV0", "OWD", "LD_LIBRARY_PATH", "QT_PLUGIN_PATH"})
            env.remove(QString::fromLatin1(name));
    }
    process.setProcessEnvironment(env);
    process.setProgram(program);
    process.setArguments(arguments);
    return process.startDetached();
}

} // namespace

bool isNewerVersion(const QString &candidate, const QString &current)
{
    const QVersionNumber next = parseVersion(candidate);
    return !next.isNull() && QVersionNumber::compare(next, parseVersion(current)) > 0;
}

QString platformPackageSuffix()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("-windows-x64.zip");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("-macos.dmg");
#else
    return QStringLiteral("-linux-x86_64.AppImage");
#endif
}

std::optional<UpdateInfo> parseLatestRelease(const QJsonObject &release, const QString &current,
                                             const QString &suffix)
{
    const QString tag = release.value("tag_name").toString();
    if (!isNewerVersion(tag, current))
        return std::nullopt;
    for (const QJsonValue &value : release.value("assets").toArray()) {
        const QJsonObject asset = value.toObject();
        const QUrl url(asset.value("browser_download_url").toString());
        if (asset.value("name").toString().endsWith(suffix) && url.scheme() == QLatin1String("https"))
            return UpdateInfo{parseVersion(tag).toString(), url,
                              QUrl(release.value("html_url").toString())};
    }
    return std::nullopt;
}

// ---------------------------------------------------------------------------

Updater::Updater(QObject *parent)
    : QObject(parent)
{
}

void Updater::check()
{
    QNetworkRequest r = request(kLatestRelease);
    r.setRawHeader("Accept", "application/vnd.github+json");
    QNetworkReply *reply = m_network.get(r);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(reply->errorString());
            return;
        }
        const auto update = parseLatestRelease(QJsonDocument::fromJson(reply->readAll()).object(),
                                               QCoreApplication::applicationVersion(),
                                               platformPackageSuffix());
        if (update)
            emit updateAvailable(*update);
        else
            emit upToDate();
    });
}

void Updater::download(const UpdateInfo &update)
{
    // AppImage : téléchargée à côté de l'ancienne pour la remplacer d'un coup.
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    const QString dir = appImage.isEmpty() ? QDir::tempPath() : QFileInfo(appImage).absolutePath();
    const QString path = QDir(dir).filePath(update.package.fileName() + QStringLiteral(".part"));
    auto file = std::make_shared<QFile>(path);
    if (!file->open(QIODevice::WriteOnly)) {
        emit failed(tr("Impossible d'écrire %1 : %2").arg(path, file->errorString()));
        return;
    }
    QNetworkReply *reply = m_network.get(request(update.package));
    connect(reply, &QNetworkReply::readyRead, this, [reply, file] { file->write(reply->readAll()); });
    connect(reply, &QNetworkReply::downloadProgress, this, &Updater::progress);
    connect(reply, &QNetworkReply::finished, this, [this, reply, file] {
        reply->deleteLater();
        file->write(reply->readAll());
        file->close();
        if (reply->error() != QNetworkReply::NoError) {
            file->remove();
            emit failed(reply->errorString());
            return;
        }
        // Nom définitif (Expand-Archive exige l'extension .zip).
        QString path = file->fileName();
        path.chop(5);
        QFile::remove(path);
        if (!file->rename(path)) {
            emit failed(tr("Impossible de renommer %1.").arg(file->fileName()));
            return;
        }
        emit downloaded(path);
    });
}

bool Updater::canSelfUpdate()
{
#if defined(Q_OS_WIN)
    return QFileInfo(QCoreApplication::applicationDirPath()).isWritable();
#elif defined(Q_OS_MACOS)
    const QString bundle = appBundle();
    return !bundle.isEmpty() && QFileInfo(QFileInfo(bundle).absolutePath()).isWritable();
#else
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    return !appImage.isEmpty() && QFileInfo(QFileInfo(appImage).absolutePath()).isWritable();
#endif
}

bool Updater::installAndRestart(const QString &package, QString *error)
{
    const QString pid = QString::number(QCoreApplication::applicationPid());
#if defined(Q_OS_WIN)
    // Les fichiers d'un programme en cours d'exécution sont verrouillés : un
    // script attend la fermeture du jeu, écrase ses fichiers puis le relance.
    auto quote = [](QString s) {
        return QStringLiteral("'%1'").arg(s.replace(QLatin1Char('\''), QStringLiteral("''")));
    };
    const QString dir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
    const QString script
        = QStringLiteral("Wait-Process -Id %1 -ErrorAction SilentlyContinue; "
                         "Expand-Archive -LiteralPath %2 -DestinationPath %3 -Force; "
                         "Remove-Item -LiteralPath %2; Start-Process -FilePath %4")
              .arg(pid, quote(QDir::toNativeSeparators(package)), quote(dir),
                   quote(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())));
    const bool started = QProcess::startDetached(
        QStringLiteral("powershell"),
        {QStringLiteral("-NoProfile"), QStringLiteral("-ExecutionPolicy"), QStringLiteral("Bypass"),
         QStringLiteral("-WindowStyle"), QStringLiteral("Hidden"), QStringLiteral("-Command"),
         script});
#elif defined(Q_OS_MACOS)
    // Après la fermeture du jeu : copie du nouveau bundle à côté de l'ancien,
    // puis remplacement (l'ancien n'est supprimé que si la copie a réussi).
    const QString script = QStringLiteral(
        "while kill -0 \"$1\" 2>/dev/null; do sleep 0.5; done\n"
        "mnt=$(mktemp -d) || exit 1\n"
        "hdiutil attach -nobrowse -readonly -mountpoint \"$mnt\" \"$2\" >/dev/null || exit 1\n"
        "app=$(ls -d \"$mnt\"/*.app | head -n 1)\n"
        "if [ -n \"$app\" ] && cp -R \"$app\" \"$3.new\"; then rm -rf \"$3\" && mv \"$3.new\" \"$3\"; fi\n"
        "hdiutil detach \"$mnt\" >/dev/null\n"
        "rm -f \"$2\"\n"
        "open \"$3\"\n");
    const bool started = QProcess::startDetached(
        QStringLiteral("/bin/sh"), {QStringLiteral("-c"), script, QStringLiteral("sh"), pid, package,
                                    appBundle()});
#else
    // Une AppImage en cours d'exécution peut être remplacée (rename atomique).
    Q_UNUSED(pid);
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    QFile::setPermissions(package, QFile::permissions(package) | QFile::ExeOwner
                                       | QFile::ExeGroup | QFile::ExeOther);
    std::error_code ec;
    std::filesystem::rename(QFile::encodeName(package).toStdString(),
                            QFile::encodeName(appImage).toStdString(), ec);
    if (ec) {
        if (error)
            *error = QString::fromStdString(ec.message());
        return false;
    }
    const bool started = launch(appImage, {});
#endif
    if (!started && error)
        *error = QObject::tr("Impossible de lancer l'installation.");
    return started;
}

namespace {
/// Emplacement d'installation de l'AppImage (convention des AppImage) et entrée du menu.
QString installedAppImage() { return QDir::homePath() + QStringLiteral("/Applications/TopArena.AppImage"); }
QString desktopEntry()
{
    return QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)
           + QStringLiteral("/top-arena.desktop");
}

bool writeDesktopEntry(QString *error)
{
    const QString icons = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                          + QStringLiteral("/icons/hicolor/256x256/apps");
    QDir().mkpath(icons);
    QFile::remove(icons + QStringLiteral("/top-arena.png"));
    QFile::copy(QStringLiteral(":/app-icon.png"), icons + QStringLiteral("/top-arena.png"));

    QDir().mkpath(QFileInfo(desktopEntry()).absolutePath());
    QFile file(desktopEntry());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error)
            *error = file.errorString();
        return false;
    }
    const QString exec = QStringLiteral("\"%1\"").arg(installedAppImage());
    file.write(QStringLiteral("[Desktop Entry]\nType=Application\nName=Top Arena\n"
                              "Comment=Combats de toupies en 2D, en solo contre des bots ou en réseau local\n"
                              "Comment[en]=2D spinning top battles, solo against bots or over a local network\n"
                              "Exec=%1\nTryExec=%2\nIcon=top-arena\nCategories=Game;ArcadeGame;\nTerminal=false\n")
                   .arg(exec, installedAppImage())
                   .toUtf8());
    return true;
}
} // namespace

bool Updater::canInstallAppImage()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    return false;
#else
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    return !appImage.isEmpty()
           && QFileInfo(appImage).canonicalFilePath() != QFileInfo(installedAppImage()).canonicalFilePath();
#endif
}

bool Updater::installAppImage(QString *error)
{
    const QString target = installedAppImage();
    QDir().mkpath(QFileInfo(target).absolutePath());
    // Copie à côté puis renommage : une AppImage déjà installée n'est jamais à moitié écrite.
    const QString part = target + QStringLiteral(".part");
    QFile::remove(part);
    if (!QFile::copy(qEnvironmentVariable("APPIMAGE"), part)) {
        if (error)
            *error = QObject::tr("Impossible de copier l'application dans %1.").arg(QFileInfo(target).absolutePath());
        return false;
    }
    QFile::setPermissions(part, QFile::permissions(part) | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
    std::error_code ec;
    std::filesystem::rename(QFile::encodeName(part).toStdString(), QFile::encodeName(target).toStdString(), ec);
    if (ec) {
        if (error)
            *error = QString::fromStdString(ec.message());
        return false;
    }
    if (!writeDesktopEntry(error))
        return false;
    if (!launch(target, {})) {
        if (error)
            *error = QObject::tr("Impossible de lancer %1.").arg(target);
        return false;
    }
    return true;
}

void Updater::ensureDesktopEntry()
{
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    const QString appImage = qEnvironmentVariable("APPIMAGE");
    if (!appImage.isEmpty() && !canInstallAppImage() && !QFile::exists(desktopEntry()))
        writeDesktopEntry(nullptr);
#endif
}

bool Updater::restart(QString *error)
{
    QString program = QCoreApplication::applicationFilePath();
#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS)
    if (const QString appImage = qEnvironmentVariable("APPIMAGE"); !appImage.isEmpty())
        program = appImage; // Le binaire monté disparaît avec l'instance actuelle.
#endif
    const bool started = launch(program, QCoreApplication::arguments().mid(1));
    if (!started && error)
        *error = QObject::tr("Impossible de relancer le jeu.");
    return started;
}

} // namespace toupie::net
