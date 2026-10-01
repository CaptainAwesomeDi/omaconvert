#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "unitcatalog.h"

// The QML facade, shaped like Omacalc's Backend: an entry machine fed by
// pressKey, everything else derived. The entry is the single source of
// truth; the result, statement and rate note are computed properties, so
// any change (typing, swapping, picking units) re-renders them.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString categoryId READ categoryId NOTIFY categoryChanged)
    Q_PROPERTY(QString categoryName READ categoryName NOTIFY categoryChanged)
    Q_PROPERTY(QString fromId READ fromId NOTIFY unitsChanged)
    Q_PROPERTY(QString toId READ toId NOTIFY unitsChanged)
    Q_PROPERTY(QString fromSymbol READ fromSymbol NOTIFY unitsChanged)
    Q_PROPERTY(QString toSymbol READ toSymbol NOTIFY unitsChanged)
    Q_PROPERTY(QString fromName READ fromName NOTIFY unitsChanged)
    Q_PROPERTY(QString toName READ toName NOTIFY unitsChanged)
    Q_PROPERTY(QString inputDisplay READ inputDisplay NOTIFY calculationChanged)
    Q_PROPERTY(QString display READ display NOTIFY calculationChanged)
    Q_PROPERTY(QString rateNote READ rateNote NOTIFY calculationChanged)
    Q_PROPERTY(QString focusedSide READ focusedSide WRITE setFocusedSide
               NOTIFY focusedSideChanged)
    Q_PROPERTY(QString filterPlaceholder READ filterPlaceholder CONSTANT)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    QString categoryId() const { return m_categoryId; }
    QString fromId() const { return m_fromId; }
    QString toId() const { return m_toId; }
    QString categoryName() const;
    QString fromSymbol() const;
    QString toSymbol() const;
    QString fromName() const;
    QString toName() const;
    QString inputDisplay() const;
    QString display() const;
    QString rateNote() const;
    QString focusedSide() const { return m_focusedSide; }
    // Q_INVOKABLE, not just a property WRITE accessor: QML calls it as a
    // method from openUnitPicker and the Tab handler.
    Q_INVOKABLE void setFocusedSide(const QString &side);
    QString filterPlaceholder() const;
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);

    Q_INVOKABLE void pressKey(const QString &key);
    Q_INVOKABLE void swap();
    Q_INVOKABLE void cycleUnit(int delta, const QString &side);
    Q_INVOKABLE void cycleCategory(int delta);
    Q_INVOKABLE void selectCategory(const QString &id);
    Q_INVOKABLE void selectUnit(const QString &side, const QString &unitId);
    Q_INVOKABLE QVariantList categoriesModel() const;
    Q_INVOKABLE QVariantList unitsModel() const;
    // The picker's ranked list: exact symbol match first, then prefix matches
    // on symbol, alias and name, then substrings — so "g" is gram
    // and "gi" reaches GiB.
    Q_INVOKABLE QVariantList filteredUnits(const QString &filter) const;
    Q_INVOKABLE void copyResult() const;
    Q_INVOKABLE void copyStatement() const;
    Q_INVOKABLE void pasteNumber();
    // Headless smoke support: QML console output is unreliable across
    // platforms, this always reaches stderr.
    Q_INVOKABLE void smokeReport(const QString &message) const;
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height,
                                        bool maximized);

    static QString formatQuantity(double value);

signals:
    void categoryChanged();
    void unitsChanged();
    void calculationChanged();
    void focusedSideChanged();
    void textScaleChanged();

private:
    QList<UnitCatalog::Unit> unitsForCategory(const QString &categoryId) const;
    QStringList unitIds(const QString &categoryId) const;
    QString defaultFrom(const QString &categoryId) const;
    QString defaultTo(const QString &categoryId) const;
    UnitCatalog::Unit findUnit(const QString &categoryId, const QString &unitId) const;
    void loadCategoryState();
    void loadPairState();
    void saveCategoryState() const;
    void pressDigit(const QString &digit);
    void pressDecimal();
    void pressToggleSign();
    void pressBackspace();
    void pressClear();
    QString sealedEntry() const;
    double entryValue(bool *ok) const;
    bool entryConverts() const;

    QString m_categoryId;
    QString m_fromId;
    QString m_toId;
    QString m_entry;
    bool m_landingSelected = false;  // The first digit replaces the initial 1.
    QString m_focusedSide = QStringLiteral("from");  // The editing side.
    qreal m_textScale = 1.0;
};
