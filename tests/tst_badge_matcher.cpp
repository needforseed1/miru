#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryDir>
#include <QVariantMap>

#include "../src/services/BadgeMatcher.h"

class BadgeMatcherTest : public QObject
{
    Q_OBJECT

private slots:
    void matchesEnabledRulesInGroupOrder();
    void ignoresMissingFilesAndEmptyInput();
};

namespace {
QString writeBadgeConfig(QTemporaryDir &dir)
{
    const QString path = dir.filePath(QStringLiteral("badges.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }

    file.write(R"JSON({
  "groups": [
    { "id": "quality" },
    { "id": "source" }
  ],
  "filters": [
    {
      "name": "WEB-DL",
      "groupId": "source",
      "pattern": "\\bWEB[- .]?DL\\b",
      "tagColor": "#101010",
      "borderColor": "#202020",
      "textColor": "#fefefe",
      "isEnabled": true
    },
    {
      "name": "2160p",
      "groupId": "quality",
      "pattern": "\\b2160p\\b",
      "tagColor": "#111111",
      "borderColor": "#222222",
      "textColor": "#ffffff",
      "isEnabled": true
    },
    {
      "name": "Disabled",
      "groupId": "quality",
      "pattern": "REMUX",
      "isEnabled": false
    },
    {
      "name": "Invalid",
      "groupId": "source",
      "pattern": "[",
      "isEnabled": true
    }
  ]
})JSON");
    return path;
}
}

void BadgeMatcherTest::matchesEnabledRulesInGroupOrder()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    BadgeMatcher matcher;
    matcher.load(writeBadgeConfig(dir));

    const QVariantList badges = matcher.match(QStringLiteral("Example.Movie.2160p.WEB-DL.REMUX"));
    QCOMPARE(badges.size(), 2);

    const QVariantMap first = badges.at(0).toMap();
    QCOMPARE(first.value(QStringLiteral("text")).toString(), QStringLiteral("2160p"));
    QCOMPARE(first.value(QStringLiteral("bg")).toString(), QStringLiteral("#111111"));
    QCOMPARE(first.value(QStringLiteral("border")).toString(), QStringLiteral("#222222"));
    QCOMPARE(first.value(QStringLiteral("fg")).toString(), QStringLiteral("#ffffff"));

    const QVariantMap second = badges.at(1).toMap();
    QCOMPARE(second.value(QStringLiteral("text")).toString(), QStringLiteral("WEB-DL"));
    QCOMPARE(second.value(QStringLiteral("bg")).toString(), QStringLiteral("#101010"));
}

void BadgeMatcherTest::ignoresMissingFilesAndEmptyInput()
{
    BadgeMatcher matcher;
    matcher.load(QStringLiteral("/does/not/exist.json"));

    QVERIFY(matcher.match(QString()).isEmpty());
    QVERIFY(matcher.match(QStringLiteral("Example.Movie.1080p")).isEmpty());
}

QTEST_MAIN(BadgeMatcherTest)
#include "tst_badge_matcher.moc"
