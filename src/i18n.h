#pragma once

#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QString>
#include <QStringList>

// Translations are JSON dictionaries embedded in the resources
// (i18n/omaconvert.<language>.json); the source language is the English
// string itself. The classic .ts/.qm pipeline would add a qt6-tools build
// dependency Omarchy does not ship, so the dictionaries keep i18n
// toolchain-free — swapping in QTranslator later only touches this header.
//
// The language follows the system locale, which is what Omarchy sets from
// its install-time language choice; a missing dictionary falls back to
// English by construction (translate() returns the key).
namespace I18n {

namespace detail {
inline QHash<QString, QString> &dictionary() {
    static QHash<QString, QString> map;
    static bool loaded = false;
    if (loaded)
        return map;
    loaded = true;

    const QStringList candidates = QLocale::system().uiLanguages();
    for (const QString &candidate : candidates) {
        // uiLanguages speaks BCP-47 (zh-CN); resource files carry underscore
        // names (zh_CN), so normalize before probing.
        for (const QString &name : { QString(candidate).replace(QLatin1Char('-'), QLatin1Char('_')),
                                     candidate.left(2) }) {
            if (name.isEmpty())
                continue;
            QFile file(QStringLiteral(":/i18n/omaconvert.") + name + QStringLiteral(".json"));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                continue;
            const QJsonObject json = QJsonDocument::fromJson(file.readAll()).object();
            for (auto it = json.begin(); it != json.end(); ++it)
                map.insert(it.key(), it.value().toString());
            return map;  // First matching dictionary wins; no mixing.
        }
    }
    return map;
}
}  // namespace detail

inline QString translate(const QString &source) {
    return detail::dictionary().value(source, source);
}

}  // namespace I18n
