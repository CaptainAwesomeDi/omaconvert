#include "backend.h"

#include <cstdio>

#include <QClipboard>
#include <QGuiApplication>
#include <QRect>
#include <QSettings>

#include <cmath>

#include "i18n.h"

namespace {
const auto windowGeometrySetting = QStringLiteral("window/geometry");
const auto categorySetting = QStringLiteral("state/category");
const QString fromSetting(const QString &categoryId) {
    return QStringLiteral("units/") + categoryId + QStringLiteral("/from");
}
const QString toSetting(const QString &categoryId) {
    return QStringLiteral("units/") + categoryId + QStringLiteral("/to");
}

// Digits are entered raw, so "5." and "-" can linger while typing. Seal them
// into plain numbers before they meet the math.
QString sealNumber(const QString &entry) {
    QString sealed = entry;
    if (sealed.endsWith(QLatin1Char('.')))
        sealed.chop(1);
    if (sealed.isEmpty() || sealed == QStringLiteral("-"))
        return QStringLiteral("0");
    return sealed;
}

// A plain decimal the entry machine can edit: the keypad has no exponent
// key, so a pasted "1e-7" must land as "0.0000001" or the next digit would
// grow the exponent. Scientific strings expand by shifting the decimal
// point of the significant digits — 'f' formatting would silently round
// tiny magnitudes like 1e-16 down to zero.
QString plainDecimal(double value) {
    if (value == 0)
        return QStringLiteral("0");
    if (std::abs(value) >= 1e15)
        return QString::number(value, 'f', 0);

    QString text = QString::number(value, 'g', 15);
    const int e = text.indexOf(QLatin1Char('e'));
    if (e < 0)
        return text;

    const bool negative = text.startsWith(QLatin1Char('-'));
    if (negative)
        text.remove(0, 1);

    const QString mantissa = text.left(e);
    const int exponent = text.mid(e + 1).toInt();
    QString digits = mantissa;
    digits.remove(QLatin1Char('.'));
    int point = mantissa.contains(QLatin1Char('.'))
        ? mantissa.indexOf(QLatin1Char('.')) : mantissa.size();
    point += exponent;

    QString expanded;
    if (point <= 0) {
        expanded = QStringLiteral("0.") + QString(-point, QLatin1Char('0')) + digits;
    } else if (point >= digits.size()) {
        expanded = digits + QString(point - digits.size(), QLatin1Char('0'));
    } else {
        expanded = digits.left(point) + QLatin1Char('.') + digits.mid(point);
    }
    return negative ? QLatin1Char('-') + expanded : expanded;
}
}  // namespace

Backend::Backend(QObject *parent) : QObject(parent) {
    loadCategoryState();

    m_entry = QStringLiteral("1");
    m_landingSelected = true;
}

QString Backend::categoryName() const {
    const UnitCatalog::Category *category = UnitCatalog::category(m_categoryId);
    return category ? I18n::translate(category->name) : QString();
}

QList<UnitCatalog::Unit> Backend::unitsForCategory(const QString &categoryId) const {
    const UnitCatalog::Category *category = UnitCatalog::category(categoryId);
    return category ? category->units : QList<UnitCatalog::Unit>();
}

QStringList Backend::unitIds(const QString &categoryId) const {
    QStringList ids;
    for (const UnitCatalog::Unit &unit : unitsForCategory(categoryId))
        ids << unit.id;
    return ids;
}

// The demonstrated pair on first run, and the fallback for a corrupt or
// hand-edited settings entry.
QString Backend::defaultFrom(const QString &categoryId) const {
    if (categoryId == QStringLiteral("length"))
        return QStringLiteral("m");
    if (categoryId == QStringLiteral("temperature"))
        return QStringLiteral("c");
    if (categoryId == QStringLiteral("volume"))
        return QStringLiteral("l");
    if (categoryId == QStringLiteral("data"))
        return QStringLiteral("gb");
    return QStringLiteral("kg");
}

QString Backend::defaultTo(const QString &categoryId) const {
    if (categoryId == QStringLiteral("length"))
        return QStringLiteral("ft");
    if (categoryId == QStringLiteral("temperature"))
        return QStringLiteral("f");
    if (categoryId == QStringLiteral("volume"))
        return QStringLiteral("cup");
    if (categoryId == QStringLiteral("data"))
        return QStringLiteral("gib");
    return QStringLiteral("lb");
}

UnitCatalog::Unit Backend::findUnit(const QString &categoryId, const QString &unitId) const {
    for (const UnitCatalog::Unit &unit : unitsForCategory(categoryId)) {
        if (unit.id == unitId)
            return unit;
    }
    return UnitCatalog::Unit();
}

QString Backend::fromSymbol() const {
    return findUnit(m_categoryId, m_fromId).symbol;
}

QString Backend::toSymbol() const {
    return findUnit(m_categoryId, m_toId).symbol;
}

QString Backend::fromName() const {
    return findUnit(m_categoryId, m_fromId).name;
}

QString Backend::toName() const {
    return findUnit(m_categoryId, m_toId).name;
}

QString Backend::inputDisplay() const {
    return m_entry.isEmpty() ? QStringLiteral("0") : m_entry;
}

QString Backend::display() const {
    const UnitCatalog::Unit from = findUnit(m_categoryId, m_fromId);
    const UnitCatalog::Unit to = findUnit(m_categoryId, m_toId);
    if (from.id.isEmpty() || to.id.isEmpty())
        return QStringLiteral("0");

    // A same-unit pair is the identity: show the entry untouched so a value
    // like 0.1 never takes a float round-trip.
    if (from.id == to.id)
        return inputDisplay();

    bool valueOk = false;
    const double value = entryValue(&valueOk);
    if (!valueOk || !qIsFinite(value))
        return QStringLiteral("Error");

    bool convertOk = false;
    bool belowAbsoluteZero = false;
    const double result = UnitCatalog::convert(value, from, to, &convertOk,
                                               &belowAbsoluteZero);
    if (!convertOk)
        return belowAbsoluteZero ? QStringLiteral("Below absolute zero")
                                 : QStringLiteral("Error");
    return formatQuantity(result);
}

QString Backend::rateNote() const {
    const UnitCatalog::Unit from = findUnit(m_categoryId, m_fromId);
    const UnitCatalog::Unit to = findUnit(m_categoryId, m_toId);
    if (from.id.isEmpty() || to.id.isEmpty())
        return QString();

    // "1 °C = 33.8 °F" would read as a ratio and teach the wrong arithmetic;
    // the affine categories show no teacher line.
    if (from.offset != 0 || to.offset != 0)
        return QString();

    const double factor = UnitCatalog::convert(1, from, to);
    return QStringLiteral("1 %1 = %2 %3")
        .arg(from.symbol, QString::number(factor, 'g', 10), to.symbol);
}

QString Backend::filterPlaceholder() const {
    return I18n::translate(QStringLiteral("Filter…"));
}

void Backend::setFocusedSide(const QString &side) {
    if ((side == QStringLiteral("from") || side == QStringLiteral("to"))
            && side != m_focusedSide) {
        m_focusedSide = side;
        emit focusedSideChanged();
    }
}

void Backend::pressKey(const QString &key) {
    if (key.size() == 1 && key.at(0).isDigit()) {
        pressDigit(key);
    } else if (key == QStringLiteral(".")) {
        pressDecimal();
    } else if (key == QStringLiteral("sign")) {
        pressToggleSign();
    } else if (key == QStringLiteral("backspace")) {
        pressBackspace();
    } else if (key == QStringLiteral("clear")) {
        pressClear();
    } else {
        return;
    }

    emit calculationChanged();
}

void Backend::pressDigit(const QString &digit) {
    // A digit recovers from an unconvertible entry (an overflowed paste)
    // instead of appending to it, like Omacalc's error recovery. The entry
    // itself can be finite while its conversion overflows, so the check
    // rides the same pipeline the display uses.
    if (!entryConverts()) {
        m_entry.clear();
        m_landingSelected = false;
    }

    // The landing "1" behaves like selected text: the first digit replaces
    // it instead of appending.
    if (m_landingSelected) {
        m_landingSelected = false;
        m_entry.clear();
    }

    if (m_entry == QStringLiteral("0")) {
        m_entry = digit;
        return;
    }
    if (m_entry == QStringLiteral("-0")) {
        m_entry = QStringLiteral("-") + digit;
        return;
    }

    // Fifteen significant digits is what the display format preserves; the
    // zero in a leading "0." is not significant, so it does not count.
    int digits = 0;
    for (const QChar character : m_entry) {
        if (character.isDigit())
            ++digits;
    }
    if (m_entry.startsWith(QStringLiteral("0."))
            || m_entry.startsWith(QStringLiteral("-0.")))
        --digits;
    if (digits < 15)
        m_entry += digit;
}

void Backend::pressDecimal() {
    m_landingSelected = false;

    if (m_entry.isEmpty())
        m_entry = QStringLiteral("0.");
    else if (!m_entry.contains(QLatin1Char('.')))
        m_entry += QLatin1Char('.');
}

void Backend::pressToggleSign() {
    m_landingSelected = false;

    // With nothing typed, start a fresh negative operand instead of dredging
    // up the previous one.
    if (m_entry.isEmpty()) {
        m_entry = QStringLiteral("-0");
        return;
    }

    if (m_entry.startsWith(QLatin1Char('-')))
        m_entry.remove(0, 1);
    else
        m_entry.prepend(QLatin1Char('-'));
}

void Backend::pressBackspace() {
    m_landingSelected = false;

    m_entry.chop(1);
    if (m_entry == QStringLiteral("-"))
        m_entry.clear();
}

void Backend::pressClear() {
    m_landingSelected = false;
    m_entry.clear();
}

void Backend::swap() {
    std::swap(m_fromId, m_toId);
    saveCategoryState();
    // The typed number stays put; the result is never fed back in.
    emit unitsChanged();
    emit calculationChanged();
}

void Backend::cycleUnit(int delta, const QString &side) {
    const QStringList ids = unitIds(m_categoryId);
    if (ids.isEmpty())
        return;

    const QString current = side == QStringLiteral("from") ? m_fromId : m_toId;
    int index = ids.indexOf(current);
    if (index < 0)
        index = 0;
    index = (index + delta + ids.size()) % ids.size();
    selectUnit(side, ids.at(index));
}

void Backend::cycleCategory(int delta) {
    const QList<UnitCatalog::Category> &table = UnitCatalog::categories();
    int index = 0;
    for (int i = 0; i < table.size(); ++i) {
        if (table.at(i).id == m_categoryId)
            index = i;
    }
    index = (index + delta + table.size()) % table.size();
    selectCategory(table.at(index).id);
}

void Backend::selectCategory(const QString &id) {
    if (!UnitCatalog::category(id) || id == m_categoryId)
        return;

    m_categoryId = id;
    loadPairState();
    saveCategoryState();

    emit categoryChanged();
    emit unitsChanged();
    emit calculationChanged();
}

void Backend::selectUnit(const QString &side, const QString &unitId) {
    const QStringList ids = unitIds(m_categoryId);
    if (!ids.contains(unitId))
        return;

    if (side == QStringLiteral("from"))
        m_fromId = unitId;
    else if (side == QStringLiteral("to"))
        m_toId = unitId;
    else
        return;

    saveCategoryState();
    emit unitsChanged();
    emit calculationChanged();
}

QVariantList Backend::categoriesModel() const {
    QVariantList model;
    for (const UnitCatalog::Category &category : UnitCatalog::categories()) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), category.id);
        entry.insert(QStringLiteral("name"), I18n::translate(category.name));
        model.append(entry);
    }
    return model;
}

QVariantList Backend::unitsModel() const {
    QVariantList model;
    for (const UnitCatalog::Unit &unit : unitsForCategory(m_categoryId)) {
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), unit.id);
        entry.insert(QStringLiteral("symbol"), unit.symbol);
        entry.insert(QStringLiteral("name"), unit.name);
        entry.insert(QStringLiteral("aliases"), unit.aliases);
        model.append(entry);
    }
    return model;
}

QVariantList Backend::filteredUnits(const QString &filter) const {
    const QVariantList model = unitsModel();
    const QString query = filter.trimmed();
    if (query.isEmpty())
        return model;

    const QString needle = query.toLower();
    QList<QPair<int, int>> ranked;  // (score, model index); lower wins.
    for (int i = 0; i < model.size(); ++i) {
        const QVariantMap unit = model.at(i).toMap();
        const QString symbol = unit.value(QStringLiteral("symbol")).toString().toLower();
        const QString name = unit.value(QStringLiteral("name")).toString().toLower();
        int score = -1;
        if (symbol == needle)
            score = 0;
        else if (symbol.startsWith(needle))
            score = 1;
        else if (name.startsWith(needle))
            score = 2;
        else if (symbol.contains(needle))
            score = 3;
        else if (name.contains(needle))
            score = 4;
        if (score < 0) {
            const QStringList aliases = unit.value(QStringLiteral("aliases")).toStringList();
            for (const QString &alias : aliases) {
                if (alias.startsWith(needle)) {
                    score = 2;
                    break;
                }
                if (alias.contains(needle))
                    score = 4;
            }
        }
        if (score >= 0)
            ranked.append(qMakePair(score, i));
    }
    std::stable_sort(ranked.begin(), ranked.end());

    QVariantList filtered;
    for (const QPair<int, int> &item : ranked)
        filtered.append(model.at(item.second));
    return filtered;
}

void Backend::copyResult() const {
    const QString shown = display();
    // An error message is not a number; copying it would poison a paste.
    if (shown == QStringLiteral("Error")
            || shown == QStringLiteral("Below absolute zero"))
        return;
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(shown);
}

void Backend::copyStatement() const {
    const QString shown = display();
    if (shown == QStringLiteral("Error")
            || shown == QStringLiteral("Below absolute zero"))
        return;
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(QStringLiteral("%1 %2 = %3 %4")
                               .arg(inputDisplay(), fromSymbol(), shown, toSymbol()));
}

// Paste replaces the entry when the clipboard holds a number, tolerating
// surrounding whitespace, a decimal comma, and the typographic minus.
void Backend::pasteNumber() {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return;

    QString text = clipboard->text().trimmed();
    text.replace(QStringLiteral("−"), QStringLiteral("-"));
    text.remove(QLatin1Char(' '));

    bool ok = false;
    double value = text.toDouble(&ok);
    if (!ok) {
        text.replace(QLatin1Char(','), QLatin1Char('.'));
        value = text.toDouble(&ok);
    }
    if (!ok || !qIsFinite(value))
        return;

    m_landingSelected = false;
    m_entry = plainDecimal(value);
    emit calculationChanged();
}

QVariantMap Backend::windowGeometry() const {
    const QSettings settings;
    const QRect geometry = settings.value(windowGeometrySetting).toRect();
    QVariantMap map;
    // Positions can legitimately be negative on monitors left of or above
    // the primary, so validity travels separately.
    map.insert(QStringLiteral("valid"), geometry.isValid());
    map.insert(QStringLiteral("x"), geometry.x());
    map.insert(QStringLiteral("y"), geometry.y());
    map.insert(QStringLiteral("width"), geometry.width());
    map.insert(QStringLiteral("height"), geometry.height());
    map.insert(QStringLiteral("maximized"),
               settings.value(QStringLiteral("window/maximized"), false).toBool());
    return map;
}

void Backend::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(windowGeometrySetting, QRect(x, y, width, height));
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void Backend::smokeReport(const QString &message) const {
    fprintf(stderr, "smoke: %s\n", qUtf8Printable(message));
    fflush(stderr);
}

void Backend::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;

    m_textScale = textScale;
    emit textScaleChanged();
}

QString Backend::formatQuantity(double value) {
    if (value == 0)
        value = 0;  // Collapse negative zero.

    // Ten significant digits keeps binary-float noise out of the display
    // without pretending more precision than a conversion factor carries;
    // extremes fall back to scientific notation.
    return QString::number(value, 'g', 10);
}

QString Backend::sealedEntry() const {
    return sealNumber(m_entry);
}

double Backend::entryValue(bool *ok) const {
    return sealedEntry().toDouble(ok);
}

// True when the current entry converts to a printable number; false means
// the display is showing an error message.
bool Backend::entryConverts() const {
    const UnitCatalog::Unit from = findUnit(m_categoryId, m_fromId);
    const UnitCatalog::Unit to = findUnit(m_categoryId, m_toId);
    if (from.id.isEmpty() || to.id.isEmpty() || from.id == to.id)
        return true;

    bool valueOk = false;
    const double value = entryValue(&valueOk);
    if (!valueOk || !qIsFinite(value))
        return false;

    bool convertOk = false;
    UnitCatalog::convert(value, from, to, &convertOk);
    return convertOk;
}

// Startup: which category, then its pair.
void Backend::loadCategoryState() {
    const QSettings settings;
    m_categoryId = settings.value(categorySetting, QStringLiteral("weight")).toString();
    if (!UnitCatalog::category(m_categoryId))
        m_categoryId = QStringLiteral("weight");
    loadPairState();
}

// The category's remembered pair; on a category change this runs after
// m_categoryId is set, so it must not re-read the category from settings.
// A same-unit pair is legal and persists when both ids are live.
void Backend::loadPairState() {
    const QSettings settings;
    const QStringList ids = unitIds(m_categoryId);
    m_fromId = settings.value(fromSetting(m_categoryId)).toString();
    m_toId = settings.value(toSetting(m_categoryId)).toString();
    if (!ids.contains(m_fromId) || !ids.contains(m_toId)) {
        m_fromId = defaultFrom(m_categoryId);
        m_toId = defaultTo(m_categoryId);
        saveCategoryState();
    }
}

void Backend::saveCategoryState() const {
    QSettings settings;
    settings.setValue(categorySetting, m_categoryId);
    settings.setValue(fromSetting(m_categoryId), m_fromId);
    settings.setValue(toSetting(m_categoryId), m_toId);
}
