#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

// The curated unit tables. Every unit converts through its category's base
// unit with base = value * scale + offset, which is linear for everything
// except temperature; the two Fahrenheit/Celsius offsets cover that case
// without special-casing the conversion path.
namespace UnitCatalog {

struct Unit {
    QString id;        // stable settings key, e.g. "kg"
    QString symbol;    // what the chip and picker show, e.g. "kg"
    QString name;      // full English name; translated at display time
    QStringList aliases;
    double scale = 1;  // base = value * scale + offset
    double offset = 0;
};

struct Category {
    QString id;
    QString name;      // Full English name for the picker
    QList<Unit> units;
};

inline const QList<Category> &categories() {
    static const QList<Category> table = {
        {
            QStringLiteral("weight"), QStringLiteral("Weight"),
            {
                { QStringLiteral("mg"), QStringLiteral("mg"), QStringLiteral("milligram"),
                  { QStringLiteral("milligrams"), QStringLiteral("milligramme") }, 1e-6, 0 },
                { QStringLiteral("g"), QStringLiteral("g"), QStringLiteral("gram"),
                  { QStringLiteral("grams"), QStringLiteral("gramme"), QStringLiteral("grammes") }, 1e-3, 0 },
                { QStringLiteral("oz"), QStringLiteral("oz"), QStringLiteral("ounce"),
                  { QStringLiteral("ounces"), QStringLiteral("avoirdupois ounce") }, 0.028349523125, 0 },
                { QStringLiteral("lb"), QStringLiteral("lb"), QStringLiteral("pound"),
                  { QStringLiteral("pounds"), QStringLiteral("lbs"), QStringLiteral("avoirdupois pound") }, 0.45359237, 0 },
                { QStringLiteral("st"), QStringLiteral("st"), QStringLiteral("stone"),
                  { QStringLiteral("stones") }, 6.35029318, 0 },
                { QStringLiteral("kg"), QStringLiteral("kg"), QStringLiteral("kilogram"),
                  { QStringLiteral("kilograms"), QStringLiteral("kilo"), QStringLiteral("kilos") }, 1.0, 0 },
                { QStringLiteral("t"), QStringLiteral("t"), QStringLiteral("tonne"),
                  { QStringLiteral("tonnes"), QStringLiteral("metric ton") }, 1000.0, 0 },
            },
        },
        {
            QStringLiteral("length"), QStringLiteral("Length"),
            {
                { QStringLiteral("mm"), QStringLiteral("mm"), QStringLiteral("millimetre"),
                  { QStringLiteral("millimeter"), QStringLiteral("millimetres"), QStringLiteral("millimeters") }, 1e-3, 0 },
                { QStringLiteral("cm"), QStringLiteral("cm"), QStringLiteral("centimetre"),
                  { QStringLiteral("centimeter"), QStringLiteral("centimetres"), QStringLiteral("centimeters") }, 1e-2, 0 },
                { QStringLiteral("m"), QStringLiteral("m"), QStringLiteral("metre"),
                  { QStringLiteral("meter"), QStringLiteral("metres"), QStringLiteral("meters") }, 1.0, 0 },
                { QStringLiteral("km"), QStringLiteral("km"), QStringLiteral("kilometre"),
                  { QStringLiteral("kilometer"), QStringLiteral("kilometres"), QStringLiteral("kilometers") }, 1000.0, 0 },
                { QStringLiteral("in"), QStringLiteral("in"), QStringLiteral("inch"),
                  { QStringLiteral("inches"), QStringLiteral("\"") }, 0.0254, 0 },
                { QStringLiteral("ft"), QStringLiteral("ft"), QStringLiteral("foot"),
                  { QStringLiteral("feet"), QStringLiteral("'") }, 0.3048, 0 },
                { QStringLiteral("yd"), QStringLiteral("yd"), QStringLiteral("yard"),
                  { QStringLiteral("yards") }, 0.9144, 0 },
                { QStringLiteral("mi"), QStringLiteral("mi"), QStringLiteral("mile"),
                  { QStringLiteral("miles"), QStringLiteral("international mile") }, 1609.344, 0 },
            },
        },
        {
            QStringLiteral("temperature"), QStringLiteral("Temperature"),
            {
                { QStringLiteral("c"), QStringLiteral("°C"), QStringLiteral("Celsius"),
                  { QStringLiteral("celsius"), QStringLiteral("centigrade") }, 1.0, 273.15 },
                { QStringLiteral("f"), QStringLiteral("°F"), QStringLiteral("Fahrenheit"),
                  { QStringLiteral("fahrenheit") }, 5.0 / 9.0, 459.67 * 5.0 / 9.0 },
                { QStringLiteral("k"), QStringLiteral("K"), QStringLiteral("kelvin"),
                  { QStringLiteral("kelvins"), QStringLiteral("degrees kelvin") }, 1.0, 0.0 },
            },
        },
        {
            QStringLiteral("volume"), QStringLiteral("Volume"),
            {
                { QStringLiteral("ml"), QStringLiteral("mL"), QStringLiteral("millilitre"),
                  { QStringLiteral("milliliter"), QStringLiteral("ml") }, 0.001, 0 },
                { QStringLiteral("l"), QStringLiteral("L"), QStringLiteral("litre"),
                  { QStringLiteral("liter"), QStringLiteral("litres"), QStringLiteral("liters") }, 1.0, 0 },
                { QStringLiteral("tsp"), QStringLiteral("tsp"), QStringLiteral("teaspoon"),
                  { QStringLiteral("teaspoons") }, 0.00492892159375, 0 },
                { QStringLiteral("tbsp"), QStringLiteral("tbsp"), QStringLiteral("tablespoon"),
                  { QStringLiteral("tablespoons") }, 0.01478676478125, 0 },
                { QStringLiteral("cup"), QStringLiteral("US cup"), QStringLiteral("US cup"),
                  { QStringLiteral("cups"), QStringLiteral("cup") }, 0.2365882365, 0 },
                { QStringLiteral("floz"), QStringLiteral("US fl oz"), QStringLiteral("US fluid ounce"),
                  { QStringLiteral("fluid ounce"), QStringLiteral("fluid ounces"), QStringLiteral("fl oz") }, 0.0295735295625, 0 },
                { QStringLiteral("gal"), QStringLiteral("US gal"), QStringLiteral("US gallon"),
                  { QStringLiteral("gallon"), QStringLiteral("gallons"), QStringLiteral("gal") }, 3.785411784, 0 },
            },
        },
        {
            QStringLiteral("data"), QStringLiteral("Data"),
            {
                { QStringLiteral("b"), QStringLiteral("B"), QStringLiteral("byte"),
                  { QStringLiteral("bytes") }, 1.0, 0 },
                { QStringLiteral("kb"), QStringLiteral("kB"), QStringLiteral("kilobyte"),
                  { QStringLiteral("kilobytes") }, 1e3, 0 },
                { QStringLiteral("mb"), QStringLiteral("MB"), QStringLiteral("megabyte"),
                  { QStringLiteral("megabytes") }, 1e6, 0 },
                { QStringLiteral("gb"), QStringLiteral("GB"), QStringLiteral("gigabyte"),
                  { QStringLiteral("gigabytes") }, 1e9, 0 },
                { QStringLiteral("tb"), QStringLiteral("TB"), QStringLiteral("terabyte"),
                  { QStringLiteral("terabytes") }, 1e12, 0 },
                { QStringLiteral("kib"), QStringLiteral("KiB"), QStringLiteral("kibibyte"),
                  { QStringLiteral("kibibytes") }, 1024.0, 0 },
                { QStringLiteral("mib"), QStringLiteral("MiB"), QStringLiteral("mebibyte"),
                  { QStringLiteral("mebibytes") }, 1048576.0, 0 },
                { QStringLiteral("gib"), QStringLiteral("GiB"), QStringLiteral("gibibyte"),
                  { QStringLiteral("gibibytes") }, 1073741824.0, 0 },
                { QStringLiteral("tib"), QStringLiteral("TiB"), QStringLiteral("tebibyte"),
                  { QStringLiteral("tebibytes") }, 1099511627776.0, 0 },
            },
        },
    };
    return table;
}

inline const Category *category(const QString &id) {
    for (const Category &category : categories()) {
        if (category.id == id)
            return &category;
    }
    return nullptr;
}

// base = value * scaleFrom + offsetFrom; out = (base - offsetTo) / scaleTo.
// An identity pair returns the value untouched so no float lap can dirty it.
// Only temperature has offsets: a base below absolute zero is refused
// (*belowAbsoluteZero, within a 1e-9 epsilon that absorbs the float noise of
// the Fahrenheit constants), and linear pairs convert through their scale
// ratio directly so extreme magnitudes never overflow through the base.
inline double convert(double value, const Unit &from, const Unit &to, bool *ok = nullptr,
                      bool *belowAbsoluteZero = nullptr) {
    bool okDummy = false;
    bool belowDummy = false;
    if (!ok)
        ok = &okDummy;
    if (!belowAbsoluteZero)
        belowAbsoluteZero = &belowDummy;

    *ok = false;
    *belowAbsoluteZero = false;
    if (from.id == to.id) {
        *ok = true;
        return value;
    }

    if (from.offset == 0 && to.offset == 0) {
        // Ratio first: 1e300 TB in TiB overflows through value * 1e12 but
        // is finite through value * (1e12 / 2^40).
        const double out = value * (from.scale / to.scale);
        if (!qIsFinite(out))
            return 0;
        *ok = true;
        return out;
    }

    double base = value * from.scale + from.offset;
    if (base < 0) {
        if (base > -1e-9)
            base = 0;  // The Fahrenheit constants' rounding noise.
        else {
            *belowAbsoluteZero = true;
            return 0;
        }
    }

    const double out = (base - to.offset) / to.scale;
    if (!qIsFinite(out))
        return 0;

    *ok = true;
    return out;
}

}  // namespace UnitCatalog
