// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
static unsigned runJsonAggregateChecks() {
    unsigned checks = 0;
    const auto require = [&](bool condition) { ++checks; if (!condition) throw std::runtime_error("Captured aggregate assertion " + std::to_string(checks)); };
    const auto rejects = [&](const std::function<void()>& action, Aeostara::JsonErrorCode code) { ++checks; try { action(); } catch (const Aeostara::JsonError& error) { if (error.code() == code) return; throw; } throw std::runtime_error("Captured aggregate rejection assertion " + std::to_string(checks)); };
    using Aeostara::Json; using Aeostara::JsonSourceRegion; using Aeostara::JsonErrorCode;
    const std::string raw = " {\"z\":null,\"a\":[1,\"escaped\\ntext\"]} ";
    Json snapshot{{"document", Json::parse(raw)}, {"rawUtf8", Json::captured_raw_utf8(raw)}, {"sourceName", "actual"}};
    const std::vector<JsonSourceRegion> one{{"/document", "/rawUtf8"}};
    std::string canonical; snapshot.canonical_sources(one, [&](std::string_view chunk) { canonical.append(chunk); });
    require(canonical == snapshot.canonical());
    require(snapshot.at("rawUtf8").string_value() == raw);
    auto mismatch = snapshot; mismatch["document"] = false;
    rejects([&] { mismatch.canonical_sources(one, [](std::string_view) {}); }, JsonErrorCode::Type);
    rejects([&] { snapshot.canonical_sources({{"/document", "/absent"}}, [](std::string_view) {}); }, JsonErrorCode::Type);
    rejects([&] { snapshot.canonical_sources({{"/document", "/document"}}, [](std::string_view) {}); }, JsonErrorCode::InvalidPointer);
    rejects([&] { snapshot.canonical_sources({{"/document", "/rawUtf8"}, {"/document/a", "/sourceName"}}, [](std::string_view) {}); }, JsonErrorCode::InvalidPointer);
    rejects([&] { snapshot.canonical_sources(one, {}); }, JsonErrorCode::Type);
    rejects([&] { (void)Json::captured_raw_utf8("{broken}"); }, JsonErrorCode::Syntax);
    rejects([&] { (void)Json::captured_raw_utf8(std::string(Aeostara::JsonLimits::rawDocumentBytes + 1, ' ')); }, JsonErrorCode::Limit);
    Json source = Json::array(); for (unsigned i = 0; i < 8; ++i) source.push_back(std::string(262144, static_cast<char>('a' + i)));
    const auto largeRaw = source.canonical();
    require(largeRaw.size() > Aeostara::JsonLimits::decodedStringBytes);
    rejects([&] { (void)Json(largeRaw); }, JsonErrorCode::Limit);
    const auto capturedRaw = Json::captured_raw_utf8(largeRaw);
    rejects([&] { (void)capturedRaw.canonical(); }, JsonErrorCode::Limit);
    Json aggregate{{"sources", Json::array()}, {"revision", 1}}; std::vector<JsonSourceRegion> regions;
    for (unsigned i = 0; i < 9; ++i) { aggregate["sources"].push_back(Json{{"document", source}, {"rawUtf8", capturedRaw}}); const auto prefix = "/sources/" + std::to_string(i); regions.push_back({prefix + "/document", prefix + "/rawUtf8"}); }
    std::size_t bytes = 0, maxChunk = 0, calls = 0;
    aggregate.canonical_sources(regions, [&](std::string_view chunk) { bytes += chunk.size(); maxChunk = (std::max)(maxChunk, chunk.size()); ++calls; });
    require(bytes > Aeostara::JsonLimits::canonicalDocumentBytes);
    require(maxChunk <= 8192 && calls > 1);
    rejects([&] { (void)aggregate.canonical(); }, JsonErrorCode::Limit);
    Json deeplyNested = nullptr; for (unsigned i = 0; i < 63; ++i) deeplyNested = Json::array({deeplyNested});
    const auto deepRaw = deeplyNested.canonical(); Json deep{{"outer", Json{{"document", deeplyNested}, {"rawUtf8", Json::captured_raw_utf8(deepRaw)}}}};
    deep.canonical_sources({{"/outer/document", "/outer/rawUtf8"}}, [](std::string_view) {}); require(true);
    Json many{{"sources", Json::array()}}; std::vector<JsonSourceRegion> tooMany;
    for (unsigned i = 0; i < 33; ++i) { many["sources"].push_back(snapshot); const auto prefix = "/sources/" + std::to_string(i); tooMany.push_back({prefix + "/document", prefix + "/rawUtf8"}); }
    rejects([&] { many.canonical_sources(tooMany, [](std::string_view) {}); }, JsonErrorCode::Limit);
    auto oversizedMetadata = snapshot; Json nested = nullptr; for (unsigned i = 0; i < 64; ++i) nested = Json::array({nested}); oversizedMetadata["metadata"] = nested;
    rejects([&] { oversizedMetadata.canonical_sources(one, [](std::string_view) {}); }, JsonErrorCode::Limit);
    bool sinkFailed = false; try { snapshot.canonical_sources(one, [](std::string_view) { throw std::runtime_error("Actual consumer failure"); }); } catch (const std::runtime_error& error) { sinkFailed = std::string(error.what()) == "Actual consumer failure"; } require(sinkFailed);
    // Each raw source remains individually admissible; aggregate alone exceeds64MiB.
    Json almostEight = Json::array(); for (unsigned i = 0; i < 7; ++i) almostEight.push_back(std::string(1048576, 'q'));
    const auto boundedRaw = Json::captured_raw_utf8(almostEight.canonical()); Json tooLarge{{"sources", Json::array()}}; std::vector<JsonSourceRegion> largeRegions;
    for (unsigned i = 0; i < 10; ++i) { tooLarge["sources"].push_back(Json{{"document", almostEight}, {"rawUtf8", boundedRaw}}); const auto prefix = "/sources/" + std::to_string(i); largeRegions.push_back({prefix + "/document", prefix + "/rawUtf8"}); }
    rejects([&] { tooLarge.canonical_sources(largeRegions, [](std::string_view) {}); }, JsonErrorCode::Limit);
    return checks;
}
