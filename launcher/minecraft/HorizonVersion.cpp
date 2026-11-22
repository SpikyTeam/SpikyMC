// SPDX-License-Identifier: GPL-3.0-only
/*
 *  SpikyMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "HorizonVersion.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

#include "Application.h"
#include "Horizon.h"
#include "net/ApiRequest.h"
#include "net/NetJob.h"

namespace {
constexpr auto s_releasesUrl = "https://api.github.com/repos/horizon-rust/horizon/releases?per_page=100";

QString channelForTag(const QString& tag)
{
    if (tag == "nightly")
        return "nightly";
    if (tag.startsWith("nightly-"))
        return "snapshot";
    if (tag.startsWith("alpha"))
        return "alpha";
    if (tag.startsWith("beta"))
        return "beta";
    return {};
}
}  // namespace

HorizonVersionList::HorizonVersionList(QObject* parent) : BaseVersionList(parent) {}

Task::Ptr HorizonVersionList::getLoadTask(bool)
{
    auto job = makeShared<NetJob>(tr("Horizon releases"), APPLICATION->network());
    auto [request, response] = Net::ApiRequest::makeByteArray(QUrl(s_releasesUrl));
    job->addNetAction(request);
    // Connected before the version widget's own slot, so the model is filled by the time it preselects.
    connect(job.get(), &Task::succeeded, this, [this, response] { parse(*response); });
    return job;
}

void HorizonVersionList::parse(const QByteArray& data)
{
    auto doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        m_loaded = true;
        return;
    }

    const auto assetName = Horizon::platformAssetName();
    QList<BaseVersion::Ptr> versions;
    for (const auto& entry : doc.array()) {
        auto obj = entry.toObject();
        auto tag = obj.value("tag_name").toString();
        auto channel = channelForTag(tag);
        if (channel.isEmpty())
            continue;

        QJsonObject asset;
        for (const auto& assetEntry : obj.value("assets").toArray()) {
            auto candidate = assetEntry.toObject();
            if (candidate.value("name").toString() == assetName) {
                asset = candidate;
                break;
            }
        }
        if (asset.isEmpty())
            continue;

        auto sha256 = asset.value("digest").toString();
        if (!sha256.startsWith("sha256:"))
            continue;

        auto version = std::make_shared<HorizonVersion>();
        version->tag = tag;
        version->releaseName = obj.value("name").toString(tag);
        version->channel = channel;
        version->archiveUrl = asset.value("browser_download_url").toString();
        version->archiveSha256 = sha256.mid(7);
        version->published = QDateTime::fromString(obj.value("published_at").toString(), Qt::ISODate);
        version->recommended = channel == "nightly";
        versions.append(version);
    }

    beginResetModel();
    m_versions = versions;
    m_loaded = true;
    sortVersions();
    endResetModel();
}

void HorizonVersionList::sortVersions()
{
    std::sort(m_versions.begin(), m_versions.end(), [](const BaseVersion::Ptr& left, const BaseVersion::Ptr& right) {
        return static_cast<HorizonVersion*>(left.get())->published > static_cast<HorizonVersion*>(right.get())->published;
    });
}

QVariant HorizonVersionList::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= count())
        return {};

    auto version = static_cast<HorizonVersion*>(m_versions.at(index.row()).get());
    switch (role) {
        case VersionPointerRole:
            return QVariant::fromValue(m_versions.at(index.row()));
        case VersionIdRole:
            return version->tag;
        case VersionRole:
            return version->releaseName;
        case TypeRole:
            return version->channel;
        case RecommendedRole:
            return version->recommended;
        case SortRole:
            return version->published.toSecsSinceEpoch();
        default:
            return {};
    }
}

BaseVersionList::RoleList HorizonVersionList::providesRoles() const
{
    return { VersionPointerRole, VersionIdRole, VersionRole, TypeRole, RecommendedRole, SortRole };
}
