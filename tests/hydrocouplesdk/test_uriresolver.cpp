// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the URI resolver seam — what an argument does when what it was
// given is not a path.
//
// ArgumentInputType::URL has existed since v1 and isValidArgType() has
// always claimed it. Nothing fetched: every typed argument opened a local
// ifstream, so an https URL came back as "Cannot open file". These gates fix
// the meaning of URL, and fix the two ways of getting it wrong that look
// right -- treating a Windows drive letter as a scheme, and remembering a
// URL as somewhere to save to.

#include <gtest/gtest.h>

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/io/uriresolver.h"
#include "hydrocouplesdk/temporal/timeseriesargument.h"

#include <filesystem>
#include <fstream>
#include <memory>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace
{

using AT = HydroCouple::IArgument::ArgumentInputType;

std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);
    return dir / name;
}

//! A resolver that serves one canned body and counts what it was asked for.
class FakeResolver : public UriResolver
{
public:
    explicit FakeResolver(std::string body, std::string scheme = "https")
        : m_body(std::move(body)), m_scheme(std::move(scheme))
    {
    }

    [[nodiscard]] bool handles(const std::string &scheme) const override
    {
        return scheme == m_scheme;
    }

    [[nodiscard]] bool resolve(const std::string &uri, std::string &contents,
                               std::string &message) override
    {
        ++asked;
        lastUri = uri;

        if (m_refuse)
        {
            message = "the service answered 503";
            return false;
        }

        contents = m_body;
        return true;
    }

    void refuse() { m_refuse = true; }

    int asked = 0;
    std::string lastUri;

private:
    std::string m_body;
    std::string m_scheme;
    bool m_refuse = false;
};

//! Installs a resolver for one test and takes it away again.
class InstalledResolver
{
public:
    explicit InstalledResolver(UriResolver *resolver) { setUriResolver(resolver); }
    ~InstalledResolver() { setUriResolver(nullptr); }
};

class UriResolverTest : public ::testing::Test
{
protected:
    void SetUp() override { setUriResolver(nullptr); }
    void TearDown() override { setUriResolver(nullptr); }

    Dimension m_dim{"cell", "Cell index"};
    Dimension m_timeDim{"time", "Time index"};
};

} // namespace

// ── What is a scheme ──────────────────────────────────────────────────────

// RFC 3986 permits a one-letter scheme. Nobody uses one, and every Windows
// path starts with a drive letter and a colon, so a one-letter scheme would
// send C:\data\flow.json to a resolver.
TEST_F(UriResolverTest, AWindowsDriveLetterIsNotAScheme)
{
    EXPECT_EQ(uriScheme("C:\\data\\flow.json"), "");
    EXPECT_EQ(uriScheme("c:/data/flow.json"), "");
}

TEST_F(UriResolverTest, APlainPathHasNoScheme)
{
    EXPECT_EQ(uriScheme("/usr/local/share/flow.json"), "");
    EXPECT_EQ(uriScheme("flow.json"), "");
    EXPECT_EQ(uriScheme("./relative/flow.json"), "");
}

TEST_F(UriResolverTest, ASchemeIsRecognisedWhateverItsCase)
{
    EXPECT_EQ(uriScheme("https://example.org/flow.json"), "https");
    EXPECT_EQ(uriScheme("HTTPS://example.org/flow.json"), "https");
    EXPECT_EQ(uriScheme("file:///data/flow.json"), "file");
}

// ── What the SDK can do with no host help at all ──────────────────────────

TEST_F(UriResolverTest, APlainPathIsReadWithoutAResolver)
{
    const auto path = artifactPath("uri_plain.json");
    {
        std::ofstream f(path);
        f << "[1, 2, 3]";
    }

    std::string contents;
    std::string message;
    ASSERT_TRUE(readReference(path.string(), contents, message)) << message;
    EXPECT_EQ(contents, "[1, 2, 3]");
}

TEST_F(UriResolverTest, AFileUriIsReadWithoutAResolver)
{
    const auto path = artifactPath("uri_file_scheme.json");
    {
        std::ofstream f(path);
        f << "[4, 5, 6]";
    }

    std::string contents;
    std::string message;

    // file:///abs/path, and the file:/abs/path form beside it.
    ASSERT_TRUE(readReference("file://" + path.generic_string(), contents,
                              message))
        << message;
    EXPECT_EQ(contents, "[4, 5, 6]");

    contents.clear();
    ASSERT_TRUE(readReference("file:" + path.generic_string(), contents,
                              message))
        << message;
    EXPECT_EQ(contents, "[4, 5, 6]");
}

TEST_F(UriResolverTest, AFileUriOnLocalhostIsStillThisMachine)
{
    const auto path = artifactPath("uri_file_localhost.json");
    {
        std::ofstream f(path);
        f << "[7]";
    }

    std::string contents;
    std::string message;
    ASSERT_TRUE(readReference("file://localhost" + path.generic_string(),
                             contents, message))
        << message;
    EXPECT_EQ(contents, "[7]");
}

TEST_F(UriResolverTest, AFileUriOnAnotherMachineIsNotThisMachinesDisk)
{
    std::string contents;
    std::string message;
    EXPECT_FALSE(
        readReference("file://fileserver/data/flow.json", contents, message));

    // Dropping the authority would leave "/data/flow.json", which also fails
    // here -- but for the wrong reason, and on a machine that happens to have
    // /data it would succeed and read a stranger's file. The refusal has to
    // be about the host, not about a missing file.
    EXPECT_EQ(message.find("Cannot open file"), std::string::npos) << message;
    EXPECT_NE(message.find("fileserver"), std::string::npos) << message;
}

// The old behaviour was to ifstream the URL and report "Cannot open file",
// which sends the reader looking for a file that was never meant to exist.
TEST_F(UriResolverTest, ARemoteUriWithNoResolverSaysThatRatherThanBlamingAFile)
{
    std::string contents;
    std::string message;
    EXPECT_FALSE(readReference("https://example.org/flow.json", contents,
                               message));
    EXPECT_NE(message.find("resolver"), std::string::npos) << message;
    EXPECT_NE(message.find("https"), std::string::npos) << message;
    EXPECT_EQ(message.find("Cannot open file"), std::string::npos) << message;
}

// A Windows file URI carries the drive letter behind a slash that is part of
// the URI syntax and not part of the path. There is no C: on this machine, so
// what is observable here is which path the resolver decided to open.
TEST_F(UriResolverTest, AWindowsFileUriLosesTheSlashInFrontOfItsDriveLetter)
{
    std::string contents;
    std::string message;
    EXPECT_FALSE(readReference("file:///C:/data/flow.json", contents, message));
    EXPECT_NE(message.find("C:/data/flow.json"), std::string::npos) << message;
    EXPECT_EQ(message.find("/C:/data/flow.json"), std::string::npos) << message;
}

// ── What an installed resolver is and is not asked ────────────────────────

TEST_F(UriResolverTest, AnInstalledResolverIsAskedForTheSchemesItHandles)
{
    FakeResolver resolver("[10, 20]");
    InstalledResolver installed(&resolver);

    std::string contents;
    std::string message;
    ASSERT_TRUE(readReference("https://example.org/flow.json", contents,
                              message))
        << message;
    EXPECT_EQ(contents, "[10, 20]");
    EXPECT_EQ(resolver.asked, 1);
    EXPECT_EQ(resolver.lastUri, "https://example.org/flow.json");
}

TEST_F(UriResolverTest, AResolverIsNotAskedForASchemeItDoesNotHandle)
{
    FakeResolver resolver("[10, 20]");
    InstalledResolver installed(&resolver);

    std::string contents;
    std::string message;
    EXPECT_FALSE(readReference("ftp://example.org/flow.json", contents,
                               message));
    EXPECT_EQ(resolver.asked, 0);
    EXPECT_NE(message.find("ftp"), std::string::npos) << message;
}

// A resolver is for what the SDK cannot do, not a redirection of everything.
TEST_F(UriResolverTest, AResolverIsNotAskedForALocalPath)
{
    const auto path = artifactPath("uri_local_wins.json");
    {
        std::ofstream f(path);
        f << "[99]";
    }

    FakeResolver resolver("[10, 20]");
    InstalledResolver installed(&resolver);

    std::string contents;
    std::string message;
    ASSERT_TRUE(readReference(path.string(), contents, message)) << message;
    EXPECT_EQ(contents, "[99]");
    EXPECT_EQ(resolver.asked, 0);
}

TEST_F(UriResolverTest, AResolversRefusalIsTheReasonTheCallerGets)
{
    FakeResolver resolver("");
    resolver.refuse();
    InstalledResolver installed(&resolver);

    std::string contents;
    std::string message;
    EXPECT_FALSE(readReference("https://example.org/flow.json", contents,
                               message));
    EXPECT_EQ(message, "the service answered 503");
}

// ── What an argument does with it ─────────────────────────────────────────

TEST_F(UriResolverTest, AnArgumentReadsItsValuesFromAUrl)
{
    FakeResolver resolver("[7, 8, 9]");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize("https://example.org/flow.json", AT::URL,
                                    message))
        << message;
    EXPECT_EQ(argument.length(), 3);
    EXPECT_EQ(argument[2], 9);
    EXPECT_EQ(resolver.asked, 1);
}

TEST_F(UriResolverTest, AnArgumentSaysWhyAUrlItCannotReachFailed)
{
    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    EXPECT_FALSE(argument.initialize("https://example.org/flow.json", AT::URL,
                                     message));
    EXPECT_NE(message.find("resolver"), std::string::npos) << message;
}

// saveData() writes back to whatever initialize() remembered. A URL is not
// somewhere to write, and remembering it would put a file called
// "https:" in the working directory.
TEST_F(UriResolverTest, AValueReadFromAUrlIsNotSavedToAFileNamedAfterTheUrl)
{
    // The URI is one with an opaque path, because on POSIX a colon is an
    // ordinary filename character: remembering "sensor:flow.json" as an
    // output really does put a file called "sensor:flow.json" in the working
    // directory. An https URL would fail to write for the unrelated reason
    // that "https:/" is not a directory, which proves nothing.
    const std::filesystem::path junk{"sensor:flow.json"};
    std::filesystem::remove(junk);

    FakeResolver resolver("[7, 8, 9]", "sensor");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize("sensor:flow.json", AT::URL, message))
        << message;

    argument.saveData();

    EXPECT_FALSE(std::filesystem::exists(junk))
        << "a value fetched from a service was saved to a file named after it";
    std::filesystem::remove(junk);
}

// The disk half of that pairing must not have been broken to get there.
TEST_F(UriResolverTest, AValueReadFromAFileIsStillSavedBackToThatFile)
{
    const auto path = artifactPath("uri_savedata.json");
    {
        std::ofstream f(path);
        f << "[1, 2, 3]";
    }

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize(path.string(), AT::File, message))
        << message;

    argument[0] = 42;
    argument.saveData();

    std::ifstream reopened(path);
    nlohmann::json json;
    reopened >> json;
    EXPECT_EQ(json[0].get<int>(), 42);
}

// A served CSV carries a query string, and
// filesystem::path("...flow.csv?token=x").extension() is ".csv?token=x" --
// so the series would be handed to the JSON parser.
TEST_F(UriResolverTest, ASeriesServedAsCsvIsParsedAsCsvDespiteItsQueryString)
{
    FakeResolver resolver("time,value\n0.0,5.0\n1.0,7.0\n");
    InstalledResolver installed(&resolver);

    HydroCouple::SDK::Temporal::TimeSeriesArgumentDouble argument("series", "Series", nullptr,
                                                &m_timeDim);

    std::string message;
    ASSERT_TRUE(argument.initialize(
        "https://example.org/flow.csv?token=abc", AT::URL, message))
        << message;
    EXPECT_DOUBLE_EQ(argument.getInterpolatedValue(0.5), 6.0);
}

// ── Across a save and an open ─────────────────────────────────────────────
//
// The composition document reaches an argument through
// initialize(payload, JSON, ...) and nothing else, so a fetched argument
// that recorded only its values would reopen with no memory of where they
// came from. A payload may name a URI instead.

TEST_F(UriResolverTest, APayloadThatNamesAUriIsThatUriRead)
{
    FakeResolver resolver("[7, 8, 9]");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize(
        R"({"@uri": "https://example.org/flow.json"})", AT::JSON, message))
        << message;

    EXPECT_EQ(argument.length(), 3);
    EXPECT_EQ(argument[2], 9);
    EXPECT_EQ(resolver.asked, 1);
}

TEST_F(UriResolverTest, AnArgumentReadFromAServiceRecordsWhereItCameFrom)
{
    FakeResolver resolver("[7, 8, 9]");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize("https://example.org/flow.json", AT::URL,
                                    message))
        << message;

    std::string serialized;
    ASSERT_TRUE(argument.serialize(AT::JSON, serialized, message)) << message;

    const auto json = nlohmann::json::parse(serialized);
    ASSERT_TRUE(json.contains("@uri"))
        << "the composition would reopen with no idea where this came from";
    EXPECT_EQ(json["@uri"], "https://example.org/flow.json");
}

// The whole point: what one argument wrote, the next one can be opened from.
TEST_F(UriResolverTest, AFetchedArgumentSurvivesASaveAndAnOpen)
{
    FakeResolver resolver("[7, 8, 9]");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));

    std::string saved;
    {
        Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);
        std::string message;
        ASSERT_TRUE(argument.initialize("https://example.org/flow.json",
                                        AT::URL, message))
            << message;
        ASSERT_TRUE(argument.serialize(AT::JSON, saved, message)) << message;
    }

    Argument1DInt reopened("flow", &m_dim, 0, quantity.get(), nullptr);
    std::string message;
    ASSERT_TRUE(reopened.initialize(saved, AT::JSON, message)) << message;

    EXPECT_EQ(reopened.length(), 3);
    EXPECT_EQ(reopened[2], 9);
    EXPECT_EQ(resolver.asked, 2) << "the reopen did not go back to the service";
}

// A stale copy of live data, used silently because the gauge was
// unreachable, is worse than a composition that refuses to open.
TEST_F(UriResolverTest, AUriThatCannotBeReadFailsRatherThanUsingTheValuesBesideIt)
{
    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    FakeResolver resolver("");
    resolver.refuse();
    InstalledResolver installed(&resolver);

    std::string message;
    EXPECT_FALSE(argument.initialize(
        R"({"@uri": "https://example.org/flow.json", "values": [1, 2, 3]})",
        AT::JSON, message));
    EXPECT_EQ(message, "the service answered 503");
    EXPECT_EQ(argument.length(), 0) << "it ran on the values beside the URI";
}

TEST_F(UriResolverTest, ValuesGivenOutrightStopClaimingAUri)
{
    FakeResolver resolver("[7, 8, 9]");
    InstalledResolver installed(&resolver);

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize("https://example.org/flow.json", AT::URL,
                                    message))
        << message;
    ASSERT_TRUE(argument.initialize("[1, 2]", AT::JSON, message)) << message;

    std::string serialized;
    ASSERT_TRUE(argument.serialize(AT::JSON, serialized, message)) << message;
    EXPECT_FALSE(nlohmann::json::parse(serialized).contains("@uri"))
        << "hand-edited values still claim to have come from a service";
}

// A file has always been inlined, and a document can be moved next to one.
TEST_F(UriResolverTest, ALocalFileIsInlinedRatherThanRecordedAsAReference)
{
    const auto path = artifactPath("uri_not_a_reference.json");
    {
        std::ofstream f(path);
        f << "[1, 2, 3]";
    }

    auto quantity = std::unique_ptr<Quantity>(Quantity::unitLess("Flow"));
    Argument1DInt argument("flow", &m_dim, 0, quantity.get(), nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize(path.string(), AT::File, message))
        << message;

    std::string serialized;
    ASSERT_TRUE(argument.serialize(AT::JSON, serialized, message)) << message;

    const auto json = nlohmann::json::parse(serialized);
    EXPECT_FALSE(json.contains("@uri"));
    EXPECT_EQ(json["values"], nlohmann::json::parse("[1,2,3]"));
}

// Text that is not JSON at all must not be mistaken for a reference; the
// argument's own parser gives a far better message than this check could.
TEST_F(UriResolverTest, TextThatIsNotJsonIsNotMistakenForAReference)
{
    std::string uri = "untouched";
    EXPECT_FALSE(jsonUriReference("not json at all", uri));
    EXPECT_FALSE(jsonUriReference("[1, 2, 3]", uri));
    EXPECT_FALSE(jsonUriReference(R"({"@uri": ""})", uri));
    EXPECT_FALSE(jsonUriReference(R"({"@uri": 7})", uri));
    EXPECT_EQ(uri, "untouched");

    ASSERT_TRUE(jsonUriReference(R"({"@uri": "sensor:flow.json"})", uri));
    EXPECT_EQ(uri, "sensor:flow.json");
}
