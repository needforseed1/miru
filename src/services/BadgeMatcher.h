#pragma once

#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QVariantList>

// Loads a "ColorfulAndConcise"-style badge config (groups + regex filters)
// and produces the matching badges for a given release name.
class BadgeMatcher
{
public:
    void load(const QString &resourcePath);

    // Returns badges whose pattern matches text, ordered by group then by
    // their order in the config. Each badge is a map: text, bg, border, fg.
    QVariantList match(const QString &text) const;

private:
    struct Rule {
        QString name;
        QRegularExpression regex;
        QString background;
        QString border;
        QString foreground;
    };

    QList<Rule> m_rules; // pre-sorted by group order, then config order
};
