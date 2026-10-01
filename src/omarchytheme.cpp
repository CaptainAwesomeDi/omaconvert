#include "omarchytheme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QTextStream>

OmarchyTheme::OmarchyTheme(QObject *parent) : QObject(parent) {
    load();
    watch();
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this]() {
        load();
        watch();  // Editors rewrite by rename, so re-arm the watched paths.
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        load();
        watch();
    });
}

void OmarchyTheme::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    load();
    emit darkModeChanged();
}

void OmarchyTheme::load() {
    // Monochrome fallbacks for a machine without an Omarchy theme.
    m_background = m_darkMode ? QStringLiteral("#101010") : QStringLiteral("#ffffff");
    m_foreground = m_darkMode ? QStringLiteral("#eeeeee") : QStringLiteral("#222324");
    m_accent = m_darkMode ? QStringLiteral("#5584aa") : QStringLiteral("#2077b2");
    m_selection = m_darkMode ? QStringLiteral("#186a9a") : QStringLiteral("#2077b2");

    const QString colorsPath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
    QString themeMode;
    QFile file(colorsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            const int equals = line.indexOf(QLatin1Char('='));
            if (equals < 0)
                continue;

            const QString key = line.left(equals).trimmed();
            QString value = line.mid(equals + 1).trimmed();
            if (value.size() >= 2
                    && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                        || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
                value = value.mid(1, value.size() - 2);

            if (key == QStringLiteral("mode"))
                themeMode = value;
            else if (key == QStringLiteral("background"))
                m_background = value;
            else if (key == QStringLiteral("foreground"))
                m_foreground = value;
            else if (key == QStringLiteral("accent"))
                m_accent = value;
            else if (key == QStringLiteral("selection"))
                m_selection = value;
        }
    }

    // The theme file's own mode wins over the portal when it disagrees, so a
    // dark Omarchy theme never renders on light portal defaults.
    bool themeModeKnown = false;
    bool themeIsDark = m_darkMode;
    if (themeMode == QStringLiteral("dark")) {
        themeIsDark = true;
        themeModeKnown = true;
    } else if (themeMode == QStringLiteral("light")) {
        themeIsDark = false;
        themeModeKnown = true;
    } else {
        const QColor background(m_background);
        if (background.isValid()) {
            const double luminance = 0.299 * background.redF()
                + 0.587 * background.greenF() + 0.114 * background.blueF();
            themeIsDark = luminance < 0.5;
            themeModeKnown = true;
        }
    }
    if (themeModeKnown && themeIsDark != m_darkMode) {
        m_darkMode = themeIsDark;
        emit darkModeChanged();
    }

    emit colorsChanged();
}

void OmarchyTheme::watch() {
    const QStringList watched = m_watcher.files() + m_watcher.directories();
    if (!watched.isEmpty())
        m_watcher.removePaths(watched);

    const QString currentDir = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current");
    const QString themeDir = currentDir + QStringLiteral("/theme");
    const QString colorsPath = themeDir + QStringLiteral("/colors.toml");

    if (QDir(currentDir).exists())
        m_watcher.addPath(currentDir);
    if (QDir(themeDir).exists())
        m_watcher.addPath(themeDir);
    if (QFile::exists(colorsPath))
        m_watcher.addPath(colorsPath);
}
