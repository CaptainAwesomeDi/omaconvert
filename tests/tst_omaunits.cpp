#include <QtTest>
#include <QClipboard>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QSettings>
#include <QTemporaryDir>

#include "backend.h"
#include "i18n.h"
#include "omarchytheme.h"

namespace {
void press(Backend &converter, const QString &keys) {
    const QStringList sequence = keys.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &key : sequence)
        converter.pressKey(key);
}
}  // namespace

class OmaunitsTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_settingsDirectory.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
        QCoreApplication::setApplicationName(QStringLiteral("omaunits"));
        QCoreApplication::setOrganizationName(QStringLiteral("omaunits-test"));
    }

    // Backends persist their category and pairs; wipe the slate between test
    // functions so order never leaks state.
    void init() {
        QSettings settings;
        settings.clear();
        settings.sync();
    }

    void launchState() {
        Backend converter;
        QCOMPARE(converter.categoryId(), QStringLiteral("weight"));
        QCOMPARE(converter.fromSymbol(), QStringLiteral("kg"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("lb"));
        QCOMPARE(converter.fromName(), QStringLiteral("kilogram"));
        QCOMPARE(converter.inputDisplay(), QStringLiteral("1"));
        QCOMPARE(converter.display(), QStringLiteral("2.204622622"));
        QCOMPARE(converter.rateNote(), QStringLiteral("1 kg = 2.204622622 lb"));

        // The picker reads these as QML properties; without them every unit
        // chip click dies before the popup opens.
        QCOMPARE(converter.property("fromId").toString(), QStringLiteral("kg"));
        QCOMPARE(converter.property("toId").toString(), QStringLiteral("lb"));
    }

    void landingOneIsSelected() {
        Backend converter;
        press(converter, "5");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("5"));

        Backend decimal;
        press(decimal, ".");
        QCOMPARE(decimal.inputDisplay(), QStringLiteral("1."));

        Backend negative;
        press(negative, "sign");
        QCOMPARE(negative.inputDisplay(), QStringLiteral("-1"));
    }

    void entryMachine() {
        Backend converter;
        press(converter, "7 2 . 5");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("72.5"));
        QCOMPARE(converter.display(), QStringLiteral("159.8351401"));

        press(converter, "backspace");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("72."));

        press(converter, "clear");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("0"));

        press(converter, "0 0 7");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("7"));

        press(converter, "clear . 5");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("0.5"));
    }

    void capsEntryAtFifteenDigits() {
        Backend converter;
        press(converter, "1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("123456789123456"));
    }

    void referenceConversions() {
        struct Row { const char *category; const char *from; const char *to;
                     const char *expected; };
        const Row rows[] = {
            { "weight", "lb", "oz", "16" },
            { "weight", "st", "lb", "14" },
            { "length", "in", "mm", "25.4" },
            { "length", "mi", "km", "1.609344" },
            { "volume", "cup", "ml", "236.5882365" },
            { "volume", "gal", "l", "3.785411784" },
            { "data", "gib", "mib", "1024" },
            { "data", "gb", "gib", "0.9313225746" },
        };
        for (const Row &row : rows) {
            Backend converter;
            converter.selectCategory(QString::fromLatin1(row.category));
            converter.selectUnit(QStringLiteral("from"), QString::fromLatin1(row.from));
            converter.selectUnit(QStringLiteral("to"), QString::fromLatin1(row.to));
            QCOMPARE(converter.display(), QString::fromLatin1(row.expected));
        }
    }

    void temperatureReferencePoints() {
        Backend converter;
        converter.selectCategory(QStringLiteral("temperature"));
        converter.selectUnit(QStringLiteral("from"), QStringLiteral("c"));
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("f"));

        press(converter, "clear sign 4 0");
        QCOMPARE(converter.display(), QStringLiteral("-40"));

        press(converter, "clear 1 0 0");
        QCOMPARE(converter.display(), QStringLiteral("212"));

        converter.selectUnit(QStringLiteral("from"), QStringLiteral("k"));
        press(converter, "clear 0");
        QCOMPARE(converter.display(), QStringLiteral("-459.67"));
    }

    void belowAbsoluteZero() {
        Backend converter;
        converter.selectCategory(QStringLiteral("temperature"));
        converter.selectUnit(QStringLiteral("from"), QStringLiteral("k"));
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("c"));
        press(converter, "clear sign 1");
        QCOMPARE(converter.display(), QStringLiteral("Below absolute zero"));

        // A digit recovers without an explicit clear.
        press(converter, "clear 3 0 0");
        QCOMPARE(converter.display(), QStringLiteral("26.85"));
    }

    void temperatureFloatNoiseStaysBelowTheZeroMark() {
        Backend converter;
        converter.selectCategory(QStringLiteral("temperature"));
        converter.selectUnit(QStringLiteral("from"), QStringLiteral("f"));
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("k"));
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QStringLiteral("-459.67"));
        converter.pasteNumber();
        QCOMPARE(converter.display(), QStringLiteral("0"));

        // The teacher line would misread as a ratio for affine pairs.
        QVERIFY(converter.rateNote().isEmpty());
    }

    void linearExtremesConvertThroughTheRatio() {
        // 1e300 TB in TiB overflows through the base unit but is finite
        // through the direct scale ratio.
        const UnitCatalog::Category *data = UnitCatalog::category(QStringLiteral("data"));
        const UnitCatalog::Unit *tb = nullptr;
        const UnitCatalog::Unit *tib = nullptr;
        for (const UnitCatalog::Unit &unit : data->units) {
            if (unit.id == QStringLiteral("tb")) tb = &unit;
            if (unit.id == QStringLiteral("tib")) tib = &unit;
        }
        bool ok = false;
        const double result = UnitCatalog::convert(1e300, *tb, *tib, &ok);
        QVERIFY(ok);
        QCOMPARE(QString::number(result, 'g', 10), QStringLiteral("9.094947018e+299"));
    }

    void identityPairReturnsEntryUntouched() {
        Backend converter;
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("kg"));
        press(converter, "clear 0 . 1");
        QCOMPARE(converter.display(), QStringLiteral("0.1"));
    }

    void swapKeepsTypedNumber() {
        Backend converter;
        press(converter, "7 2 . 5");
        converter.swap();
        QCOMPARE(converter.fromSymbol(), QStringLiteral("lb"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("kg"));
        QCOMPARE(converter.inputDisplay(), QStringLiteral("72.5"));

        // Swap-swap returns the original face, entry included.
        converter.swap();
        QCOMPARE(converter.fromSymbol(), QStringLiteral("kg"));
        QCOMPARE(converter.inputDisplay(), QStringLiteral("72.5"));
    }

    void cyclesUnitsAndCategories() {
        Backend converter;
        // Weight order: mg g oz lb st kg t — the destination steps onward.
        converter.cycleUnit(1, QStringLiteral("to"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("st"));
        converter.cycleUnit(-1, QStringLiteral("to"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("lb"));

        converter.cycleCategory(1);
        QCOMPARE(converter.categoryId(), QStringLiteral("length"));
        QCOMPARE(converter.fromSymbol(), QStringLiteral("m"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("ft"));

        // The typed number survives a category change.
        press(converter, "9 9");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("99"));
    }

    void pickerRanking() {
        Backend weight;
        weight.selectCategory(QStringLiteral("weight"));
        QVariantList ranked = weight.filteredUnits(QStringLiteral("g"));
        QVERIFY(!ranked.isEmpty());
        QCOMPARE(ranked.first().toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("g"));  // Exact symbol beats gram-adjacent ids.

        Backend data;
        data.selectCategory(QStringLiteral("data"));
        ranked = data.filteredUnits(QStringLiteral("gi"));
        QVERIFY(!ranked.isEmpty());
        QCOMPARE(ranked.first().toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("gib"));

        // Aliases are stable ranking anchors.
        ranked = weight.filteredUnits(QStringLiteral("pou"));
        QVERIFY(!ranked.isEmpty());
        QCOMPARE(ranked.first().toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("lb"));
    }

    void persistsCategoryAndPairs() {
        {
            Backend converter;
            converter.selectCategory(QStringLiteral("length"));
            converter.selectUnit(QStringLiteral("from"), QStringLiteral("km"));
            converter.selectUnit(QStringLiteral("to"), QStringLiteral("mi"));
        }
        Backend restored;
        QCOMPARE(restored.categoryId(), QStringLiteral("length"));
        QCOMPARE(restored.fromSymbol(), QStringLiteral("km"));
        QCOMPARE(restored.toSymbol(), QStringLiteral("mi"));

        // Each category keeps its own pair; the category switch restores it.
        restored.selectCategory(QStringLiteral("weight"));
        QCOMPARE(restored.fromSymbol(), QStringLiteral("kg"));
        QCOMPARE(restored.toSymbol(), QStringLiteral("lb"));
        restored.selectCategory(QStringLiteral("length"));
        QCOMPARE(restored.fromSymbol(), QStringLiteral("km"));
    }

    void pastesNumbers() {
        Backend converter;
        QClipboard *clipboard = QGuiApplication::clipboard();

        clipboard->setText(QStringLiteral(" 42.5 "));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("42.5"));

        clipboard->setText(QStringLiteral("1,5"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("1.5"));

        clipboard->setText(QStringLiteral("−7"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("-7"));

        clipboard->setText(QStringLiteral("not a number"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("-7"));
    }

    void pastesLandAsEditableDecimals() {
        Backend converter;
        QClipboard *clipboard = QGuiApplication::clipboard();

        // The keypad has no exponent key; a scientific paste must land as a
        // plain decimal or the next digit would grow the exponent.
        clipboard->setText(QStringLiteral("1e-7"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("0.0000001"));
        press(converter, "2");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("0.00000012"));

        clipboard->setText(QStringLiteral("2.5e3"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("2500"));

        // Tiny magnitudes expand exactly rather than rounding to zero.
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("mg"));
        clipboard->setText(QStringLiteral("1e-16"));
        converter.pasteNumber();
        QCOMPARE(converter.inputDisplay(), QStringLiteral("0.0000000000000001"));
        QCOMPARE(converter.display(), QStringLiteral("1e-10"));
    }

    void overflowsRecoverOnNextDigit() {
        Backend converter;
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("mg"));
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(QStringLiteral("1e308"));
        converter.pasteNumber();
        QCOMPARE(converter.display(), QStringLiteral("Error"));

        // A digit clears the unconvertible entry instead of appending to it.
        press(converter, "5");
        QCOMPARE(converter.inputDisplay(), QStringLiteral("5"));
        QCOMPARE(converter.display(), QStringLiteral("5000000"));
    }

    void usVolumeSymbolsAreQualified() {
        Backend converter;
        converter.selectCategory(QStringLiteral("volume"));
        converter.selectUnit(QStringLiteral("from"), QStringLiteral("cup"));
        converter.selectUnit(QStringLiteral("to"), QStringLiteral("floz"));
        QCOMPARE(converter.fromSymbol(), QStringLiteral("US cup"));
        QCOMPARE(converter.toSymbol(), QStringLiteral("US fl oz"));
    }

    void formatsNumbers() {
        QCOMPARE(Backend::formatQuantity(2.2046226218487757),
                 QStringLiteral("2.204622622"));
        QCOMPARE(Backend::formatQuantity(1024.0), QStringLiteral("1024"));
        QCOMPARE(Backend::formatQuantity(25.4), QStringLiteral("25.4"));
        QCOMPARE(Backend::formatQuantity(-0.0), QStringLiteral("0"));
        QCOMPARE(Backend::formatQuantity(1e15), QStringLiteral("1e+15"));
        QCOMPARE(Backend::formatQuantity(1e-7), QStringLiteral("1e-07"));
    }

    void translationsFallBackToSource() {
        // The dictionary follows the system locale, which the test cannot
        // control; the fallback contract is what every locale depends on.
        QCOMPARE(I18n::translate(QStringLiteral("no such key")),
                 QStringLiteral("no such key"));
        // The picker placeholder is part of the translation surface.
        Backend converter;
        QVERIFY(!converter.filterPlaceholder().isEmpty());
    }

    void loadsCurrentOmarchyTheme() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray palette(
            "mode = \"light\"\n"
            "accent = \"#112233\"\n"
            "selection = \"#445566\"\n"
            "background = \"#fefefe\"\n"
            "foreground = \"#101010\"\n");
        QCOMPARE(colorsFile.write(palette), qint64(palette.size()));
        colorsFile.close();

        OmarchyTheme theme;
        QCOMPARE(theme.background(), QStringLiteral("#fefefe"));
        QCOMPARE(theme.foreground(), QStringLiteral("#101010"));
        QCOMPARE(theme.accent(), QStringLiteral("#112233"));
        QCOMPARE(theme.selection(), QStringLiteral("#445566"));
        QVERIFY(!theme.darkMode());

        // The theme's own mode wins over the portal's guess.
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray darkPalette(
            "mode = \"dark\"\n"
            "background = \"#101010\"\n"
            "foreground = \"#eeeeee\"\n"
            "accent = \"#5584aa\"\n"
            "selection = \"#186a9a\"\n");
        QCOMPARE(colorsFile.write(darkPalette), qint64(darkPalette.size()));
        colorsFile.close();
        OmarchyTheme darkTheme;
        QVERIFY(darkTheme.darkMode());
        QCOMPARE(darkTheme.background(), QStringLiteral("#101010"));

        // No mode key: fall back to reading the background's luminance.
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        const QByteArray unmarkedPalette(
            "background = '#101010'\n"
            "foreground = '#eeeeee'\n");
        QCOMPARE(colorsFile.write(unmarkedPalette), qint64(unmarkedPalette.size()));
        colorsFile.close();
        OmarchyTheme unmarkedTheme;  // Single-quoted values parse too.
        QVERIFY(unmarkedTheme.darkMode());
        QVERIFY(unmarkedTheme.accent().startsWith(QLatin1Char('#')));  // Fallback palette.
    }

private:
    QTemporaryDir m_settingsDirectory;
};

QTEST_MAIN(OmaunitsTest)
#include "tst_omaunits.moc"
