// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/manifest.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace softphone::container {
namespace {

[[nodiscard]] bool readDependencies(const nlohmann::json &node, std::vector<Dependency> &out)
{
    if (node.is_null()) {
        return true;
    }
    if (!node.is_array()) {
        return false;
    }

    for (const auto &entry : node) {
        Dependency dependency;

        if (entry.is_string()) {
            dependency.id = entry.get<std::string>();
        } else if (entry.is_object()) {
            const auto id = entry.find("id");
            if (id == entry.end() || !id->is_string()) {
                return false;
            }
            dependency.id = id->get<std::string>();

            if (const auto versions = entry.find("version"); versions != entry.end()) {
                if (!versions->is_string()) {
                    return false;
                }
                const auto range = parseVersionRange(versions->get<std::string>());
                if (!range.has_value()) {
                    return false;
                }
                dependency.versions = *range;
            }
        } else {
            return false;
        }

        if (!isValidPluginId(dependency.id)) {
            return false;
        }
        out.push_back(std::move(dependency));
    }
    return true;
}

void writeDependencies(nlohmann::json &into, const char *key,
                       const std::vector<Dependency> &dependencies)
{
    if (dependencies.empty()) {
        return;
    }

    auto array = nlohmann::json::array();
    for (const auto &dependency : dependencies) {
        if (dependency.versions.unbounded()) {
            array.push_back(dependency.id);
            continue;
        }
        array.push_back(
            nlohmann::json{{"id", dependency.id}, {"version", toString(dependency.versions)}});
    }
    into[key] = std::move(array);
}

[[nodiscard]] bool namesItself(const Manifest &manifest)
{
    const auto isSelf = [&manifest](const Dependency &d) { return d.id == manifest.id; };
    return std::ranges::any_of(manifest.requirements, isSelf) ||
           std::ranges::any_of(manifest.suggestions, isSelf);
}

[[nodiscard]] bool namedTwice(const Manifest &manifest)
{
    for (const auto &required : manifest.requirements) {
        const auto same = [&required](const Dependency &d) { return d.id == required.id; };
        if (std::ranges::any_of(manifest.suggestions, same)) {
            return true;
        }
        if (std::ranges::count_if(manifest.requirements, same) > 1) {
            return true;
        }
    }
    for (const auto &suggested : manifest.suggestions) {
        const auto same = [&suggested](const Dependency &d) { return d.id == suggested.id; };
        if (std::ranges::count_if(manifest.suggestions, same) > 1) {
            return true;
        }
    }
    return false;
}

}

std::string_view toString(ManifestProblem problem) noexcept
{
    switch (problem) {
    case ManifestProblem::None:
        return "none";
    case ManifestProblem::Malformed:
        return "malformed";
    case ManifestProblem::NoId:
        return "no id";
    case ManifestProblem::BadId:
        return "bad id";
    case ManifestProblem::NoVersion:
        return "no version";
    case ManifestProblem::BadVersion:
        return "bad version";
    case ManifestProblem::BadDependency:
        return "bad dependency";
    case ManifestProblem::DependsOnItself:
        return "depends on itself";
    case ManifestProblem::DependencyTwice:
        return "dependency named twice";
    }
    return "unknown";
}

bool isValidPluginId(std::string_view id) noexcept
{
    if (id.empty() || id.size() > 40) {
        return false;
    }
    if (id.front() < 'a' || id.front() > 'z') {
        return false;
    }
    if (id.back() == '-') {
        return false;
    }
    return std::ranges::all_of(
        id, [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; });
}

ManifestProblem parseManifest(std::string_view text, Manifest &out)
{
    const auto document = nlohmann::json::parse(text, nullptr, false);
    if (document.is_discarded() || !document.is_object()) {
        return ManifestProblem::Malformed;
    }

    Manifest manifest;

    const auto id = document.find("id");
    if (id == document.end() || !id->is_string()) {
        return ManifestProblem::NoId;
    }
    manifest.id = id->get<std::string>();
    if (!isValidPluginId(manifest.id)) {
        return ManifestProblem::BadId;
    }

    const auto version = document.find("version");
    if (version == document.end() || !version->is_string()) {
        return ManifestProblem::NoVersion;
    }
    const auto parsed = parseVersion(version->get<std::string>());
    if (!parsed.has_value()) {
        return ManifestProblem::BadVersion;
    }
    manifest.version = *parsed;

    if (const auto name = document.find("name"); name != document.end() && name->is_string()) {
        manifest.name = name->get<std::string>();
    }
    if (const auto summary = document.find("summary");
        summary != document.end() && summary->is_string()) {
        manifest.summary = summary->get<std::string>();
    }

    if (!readDependencies(document.value("requires", nlohmann::json{}), manifest.requirements)) {
        return ManifestProblem::BadDependency;
    }
    if (!readDependencies(document.value("suggests", nlohmann::json{}), manifest.suggestions)) {
        return ManifestProblem::BadDependency;
    }

    if (namesItself(manifest)) {
        return ManifestProblem::DependsOnItself;
    }
    if (namedTwice(manifest)) {
        return ManifestProblem::DependencyTwice;
    }

    if (const auto required = document.find("required");
        required != document.end() && required->is_boolean()) {
        manifest.required = required->get<bool>();
    }
    if (const auto ui = document.find("ui"); ui != document.end() && ui->is_boolean()) {
        manifest.ui = ui->get<bool>();
    }

    out = std::move(manifest);
    return ManifestProblem::None;
}

std::string toJson(const Manifest &manifest)
{
    nlohmann::json document{{"id", manifest.id}, {"version", toString(manifest.version)}};
    if (!manifest.name.empty()) {
        document["name"] = manifest.name;
    }
    if (!manifest.summary.empty()) {
        document["summary"] = manifest.summary;
    }
    writeDependencies(document, "requires", manifest.requirements);
    writeDependencies(document, "suggests", manifest.suggestions);
    if (manifest.required) {
        document["required"] = true;
    }
    if (manifest.ui) {
        document["ui"] = true;
    }
    return document.dump(2);
}

}
