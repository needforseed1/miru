#include "BadgeMatcher.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

#include <algorithm>

void BadgeMatcher::load(const QString &resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();

    // Group id -> display order
    QHash<QString, int> groupOrder;
    const QJsonArray groups = root.value(QStringLiteral("groups")).toArray();
    for (int i = 0; i < groups.size(); ++i) {
        groupOrder.insert(groups.at(i).toObject().value(QStringLiteral("id")).toString(), i);
    }

    struct Pending {
        Rule rule;
        int group;
        int order;
    };

    QList<Pending> pending;
    const QJsonArray filters = root.value(QStringLiteral("filters")).toArray();
    for (int i = 0; i < filters.size(); ++i) {
        const QJsonObject f = filters.at(i).toObject();
        if (!f.value(QStringLiteral("isEnabled")).toBool()) {
            continue;
        }

        const QString pattern = f.value(QStringLiteral("pattern")).toString();
        if (pattern.isEmpty()) {
            continue;
        }

        QRegularExpression regex(pattern, QRegularExpression::UseUnicodePropertiesOption);
        if (!regex.isValid()) {
            continue;
        }
        regex.optimize();

        Rule rule;
        rule.name = f.value(QStringLiteral("name")).toString();
        rule.regex = regex;
        rule.background = f.value(QStringLiteral("tagColor")).toString(QStringLiteral("#000000"));
        rule.border = f.value(QStringLiteral("borderColor")).toString(QStringLiteral("#888888"));
        rule.foreground = f.value(QStringLiteral("textColor")).toString(QStringLiteral("#FFFFFF"));

        pending.append({rule, groupOrder.value(f.value(QStringLiteral("groupId")).toString(), 99), i});
    }

    std::stable_sort(pending.begin(), pending.end(), [](const Pending &a, const Pending &b) {
        if (a.group != b.group) {
            return a.group < b.group;
        }
        return a.order < b.order;
    });

    m_rules.clear();
    m_rules.reserve(pending.size());
    for (const Pending &p : pending) {
        m_rules.append(p.rule);
    }
}

QVariantList BadgeMatcher::match(const QString &text) const
{
    QVariantList badges;
    if (text.isEmpty()) {
        return badges;
    }

    for (const Rule &rule : m_rules) {
        if (rule.regex.match(text).hasMatch()) {
            QVariantMap badge;
            badge.insert(QStringLiteral("text"), rule.name);
            badge.insert(QStringLiteral("bg"), rule.background);
            badge.insert(QStringLiteral("border"), rule.border);
            badge.insert(QStringLiteral("fg"), rule.foreground);
            badges.append(badge);
        }
    }
    return badges;
}
