#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

// The Omarchy theme: colors read from
// ~/.local/state/omarchy/current/theme/colors.toml and re-applied live when
// that file (or the directory around it) changes. Behavior is lifted from
// Omacalc's Backend so the two faces tint identically.
class OmarchyTheme : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(QString background READ background NOTIFY colorsChanged)
    Q_PROPERTY(QString foreground READ foreground NOTIFY colorsChanged)
    Q_PROPERTY(QString accent READ accent NOTIFY colorsChanged)
    Q_PROPERTY(QString selection READ selection NOTIFY colorsChanged)

public:
    explicit OmarchyTheme(QObject *parent = nullptr);

    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    QString background() const { return m_background; }
    QString foreground() const { return m_foreground; }
    QString accent() const { return m_accent; }
    QString selection() const { return m_selection; }

signals:
    void darkModeChanged();
    void colorsChanged();

private:
    void load();
    void watch();

    bool m_darkMode = true;
    QString m_background;
    QString m_foreground;
    QString m_accent;
    QString m_selection;
    QFileSystemWatcher m_watcher;
};
