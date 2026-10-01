#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlError>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <QFile>

#include "backend.h"
#include "omarchytheme.h"
#include "systemtheme.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("omaconvert"));
    app.setDesktopFileName(QStringLiteral("omaconvert"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("omaconvert")));

    // --screenshot=<path> renders the face at the design size into a PNG
    // and exits; the QML side skips geometry restore so the frame is the
    // canonical first-run state. --open=<category|units-from|units-to>
    // lifts a picker into the frame before the grab.
    QString screenshotPath;
    QString screenshotOpen;
    const QStringList arguments = app.arguments();
    for (const QString &argument : arguments) {
        if (argument.startsWith(QStringLiteral("--screenshot=")))
            screenshotPath = argument.mid(QStringLiteral("--screenshot=").size());
        else if (argument.startsWith(QStringLiteral("--open=")))
            screenshotOpen = argument.mid(QStringLiteral("--open=").size());
    }

    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Regular.ttf"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/iAWriterMonoS-Bold.ttf"));
    app.setOrganizationName(QStringLiteral("Omacom"));
    app.setOrganizationDomain(QStringLiteral("omacom.io"));

    QQuickStyle::setStyle(QStringLiteral("Material"));

    OmarchyTheme theme(&app);
    SystemTheme systemTheme(&app);
    theme.setDarkMode(systemTheme.darkMode());
    QObject::connect(&systemTheme, &SystemTheme::darkModeChanged, &theme,
                     &OmarchyTheme::setDarkMode);

    // Carry the desktop's text scale into the default font so the chrome that
    // inherits it grows along with the face.
    const QFont interfaceFont(QStringLiteral("iA Writer Mono S"));
    const qreal basePointSize = interfaceFont.pointSizeF() > 0
        ? interfaceFont.pointSizeF()
        : app.font().pointSizeF();
    const auto applyInterfaceFont = [&app, interfaceFont, basePointSize](qreal textScale) {
        QFont scaled = interfaceFont;
        scaled.setPointSizeF(basePointSize * textScale);
        app.setFont(scaled);
    };
    applyInterfaceFont(systemTheme.textScale());

    Backend backend(&app);
    backend.setTextScale(systemTheme.textScale());
    QObject::connect(&systemTheme, &SystemTheme::textScaleChanged, &backend,
                     [&backend, applyInterfaceFont](qreal textScale) {
        applyInterfaceFont(textScale);
        backend.setTextScale(textScale);
    });

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::warnings, &app,
                     [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings)
            qWarning().noquote() << warning.toString();
    });
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.rootContext()->setContextProperty(QStringLiteral("theme"), &theme);
    engine.rootContext()->setContextProperty(QStringLiteral("screenshotPath"),
                                             screenshotPath);
    engine.rootContext()->setContextProperty(QStringLiteral("screenshotOpen"),
                                             screenshotOpen);

    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Could not load the Omaconvert interface; resource available:"
                    << QFile::exists(QStringLiteral(":/Main.qml"));
        return -1;
    }

    if (!screenshotPath.isEmpty()) {
        auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
        QTimer::singleShot(300, window, [window, screenshotPath]() {
            const QImage frame = window->grabWindow();
            if (frame.isNull() || !frame.save(screenshotPath))
                qCritical() << "Could not save the screenshot to" << screenshotPath;
            QCoreApplication::quit();
        });
    }

    return app.exec();
}
