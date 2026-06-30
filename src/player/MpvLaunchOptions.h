#pragma once

#include <QString>
#include <QStringList>
#include <QVariantMap>

struct MpvLaunchOptions
{
    QString socketPath;
    QString url;
    QString title;
    QVariantMap headers;
    QString subtitleLanguage;
    bool enableHwdec = true;
    bool enableGpuNext = false;
    bool enableHdrHint = false;
    bool enableModernz = true;
    bool startFullscreen = true;
    QStringList extraArgs;
    double startSeconds = 0.0;
    double startPercent = 0.0;
    QString modernzScriptPath;
    QString modernzFontsPath;
    bool asahiLinux = false;
};

QStringList buildMpvArguments(const MpvLaunchOptions &options);
