#include <catch2/catch_test_macros.hpp>

#include "Project.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

using moth_noised::NodeLayout;
using moth_noised::Project;
using moth_noised::ProjectTree;

namespace
{
    // From the upstream node editor's demo set: a real multi-node graph, which
    // a hand-built fixture would not be.
    constexpr const char* kMountainTerrain =
        "E@BBZEG@BD8JFgIECArXIzwECiQIw/UoPwkuAAE@BJDQAH@BC@AIEAJBw@ABZEED0KV78YZmZmPwQDmpkZPwsAAIA/HAMAAHBCBA==";
    constexpr const char* kCellularCaves =
        "FgIcCS4AAQ@BklCQs@BlRBDNzMw9G@AIMAgAw@ADgC@BCiQIzczMPgkJ@BPkIQH4XrPhjNzEw/DBIkCM3MzD4JCQ@ADBCCAE@BQzczMvhg@B/"
        "JAL/BAAL7FE4PgQKFwkNCQg@CQQQDuB4FPwt7FC4/BAOPwnU8DA==";

    ProjectTree MakeTree( const char* encoded, float x = 0.0f, float y = 0.0f )
    {
        auto parsed = moth::noise::NodeTree::FromEncodedString( encoded );
        REQUIRE( parsed.has_value() );

        ProjectTree entry;
        entry.tree = std::move( *parsed );
        entry.layout.resize( entry.tree.GetNodeCount() );
        for( size_t i = 0; i < entry.layout.size(); ++i )
        {
            entry.layout[i] = { x + static_cast<float>( i ) * 10.0f, y + static_cast<float>( i ) };
        }
        return entry;
    }

    /// A path in the system temp directory that removes itself.
    class ScratchFile
    {
    public:
        explicit ScratchFile( const char* name )
            : mPath( std::filesystem::temp_directory_path() / name )
        {
            std::filesystem::remove( mPath );
        }

        ~ScratchFile() { std::filesystem::remove( mPath ); }

        const std::filesystem::path& Path() const { return mPath; }

    private:
        std::filesystem::path mPath;
    };
}

TEST_CASE( "a project round trips through JSON", "[project]" )
{
    Project project;
    project.trees.push_back( MakeTree( kMountainTerrain, 100.0f, 200.0f ) );
    project.trees.push_back( MakeTree( kCellularCaves, -50.0f, 0.0f ) );
    project.output = 1;
    project.preview = { 4242, 1.75f, 2 };

    const auto json = project.ToJson();

    std::string error;
    auto reloaded = Project::FromJson( json, &error );
    INFO( "error: " << error );
    REQUIRE( reloaded.has_value() );

    REQUIRE( reloaded->trees.size() == 2 );
    CHECK( reloaded->output == 1 );
    CHECK( reloaded->preview.seed == 4242 );
    CHECK( reloaded->preview.scale == 1.75f );
    CHECK( reloaded->preview.genType == 2 );

    // The graphs must survive as graphs, not just as text: compare against
    // FastNoise's own encoding rather than our JSON.
    for( size_t i = 0; i < 2; ++i )
    {
        CHECK( reloaded->trees[i].tree.ToEncodedString() == project.trees[i].tree.ToEncodedString() );
        REQUIRE( reloaded->trees[i].layout.size() == project.trees[i].layout.size() );
        for( size_t n = 0; n < reloaded->trees[i].layout.size(); ++n )
        {
            CHECK( reloaded->trees[i].layout[n].x == project.trees[i].layout[n].x );
            CHECK( reloaded->trees[i].layout[n].y == project.trees[i].layout[n].y );
        }
    }

    // And writing the reloaded project must produce the same document.
    CHECK( reloaded->ToJson() == json );
}

TEST_CASE( "the graph a project writes is a plain noise tree", "[project]" )
{
    Project project;
    project.trees.push_back( MakeTree( kMountainTerrain ) );
    project.output = 0;

    const auto json = project.ToJson();
    const auto& treeJson = json.at( "trees" )[0].at( "tree" );

    // What an engine reads is a moth.noise.tree document with nothing of the
    // editor's in it. Node positions live beside it, never inside.
    CHECK( treeJson.at( "format" ) == moth::noise::kNodeTreeFormat );
    CHECK( json.at( "trees" )[0].contains( "layout" ) );
    for( const auto& node : treeJson.at( "nodes" ) )
    {
        CHECK_FALSE( node.contains( "layout" ) );
        CHECK_FALSE( node.contains( "x" ) );
    }

    std::string error;
    auto asTree = moth::noise::NodeTree::FromJson( treeJson, &error );
    INFO( "error: " << error );
    REQUIRE( asTree.has_value() );
    CHECK( asTree->ToEncodedString() == project.trees[0].tree.ToEncodedString() );
}

TEST_CASE( "layout entries line up with the tree's node order", "[project]" )
{
    Project project;
    project.trees.push_back( MakeTree( kMountainTerrain ) );
    project.output = 0;

    const auto json = project.ToJson();
    CHECK( json.at( "trees" )[0].at( "layout" ).size() == json.at( "trees" )[0].at( "tree" ).at( "nodes" ).size() );

    std::string error;
    auto reloaded = Project::FromJson( json, &error );
    INFO( "error: " << error );
    REQUIRE( reloaded.has_value() );
    CHECK( reloaded->trees[0].layout.size() == reloaded->trees[0].tree.GetNodeCount() );
}

TEST_CASE( "a bare noise tree opens as a one tree project", "[project]" )
{
    auto tree = moth::noise::NodeTree::FromEncodedString( kMountainTerrain );
    REQUIRE( tree.has_value() );
    const auto treeJson = tree->ToJson();

    std::string error;
    auto project = Project::FromJson( treeJson, &error );
    INFO( "error: " << error );
    REQUIRE( project.has_value() );

    REQUIRE( project->trees.size() == 1 );
    CHECK( project->output == 0 );
    CHECK( project->trees[0].tree.ToEncodedString() == tree->ToEncodedString() );

    // No positions were on offer, so every node gets a default one rather than
    // the layout going short and losing its alignment with the nodes.
    CHECK( project->trees[0].layout.size() == project->trees[0].tree.GetNodeCount() );
}

TEST_CASE( "an empty project round trips", "[project]" )
{
    Project project;

    const auto json = project.ToJson();
    CHECK( json.at( "output" ).is_null() );
    CHECK( json.at( "trees" ).empty() );

    std::string error;
    auto reloaded = Project::FromJson( json, &error );
    INFO( "error: " << error );
    REQUIRE( reloaded.has_value() );
    CHECK( reloaded->trees.empty() );
    CHECK( reloaded->output == -1 );
    CHECK( reloaded->GetOutput() == nullptr );
}

TEST_CASE( "an out of range output is not silently accepted", "[project]" )
{
    Project project;
    project.trees.push_back( MakeTree( kMountainTerrain ) );
    project.output = 7;

    // Writing clamps to no output rather than emitting a dangling index...
    const auto json = project.ToJson();
    CHECK( json.at( "output" ).is_null() );
    CHECK( project.GetOutput() == nullptr );

    // ...and reading one that got there another way is an error, not a guess.
    auto broken = json;
    broken["output"] = 7;

    std::string error;
    auto reloaded = Project::FromJson( broken, &error );
    CHECK_FALSE( reloaded.has_value() );
    CHECK( error.find( "out of range" ) != std::string::npos );
}

TEST_CASE( "malformed projects are rejected with a reason", "[project]" )
{
    Project valid;
    valid.trees.push_back( MakeTree( kMountainTerrain ) );
    valid.output = 0;
    const auto json = valid.ToJson();

    std::string error;

    SECTION( "not an object" )
    {
        CHECK_FALSE( Project::FromJson( nlohmann::json::array(), &error ).has_value() );
        CHECK( error.find( "object" ) != std::string::npos );
    }

    SECTION( "wrong format" )
    {
        auto broken = json;
        broken["format"] = "something.else";
        CHECK_FALSE( Project::FromJson( broken, &error ).has_value() );
        CHECK( error.find( "format" ) != std::string::npos );
    }

    SECTION( "a version this build does not read" )
    {
        auto broken = json;
        broken["version"] = moth_noised::kProjectVersion + 1;
        CHECK_FALSE( Project::FromJson( broken, &error ).has_value() );
        CHECK( error.find( "version" ) != std::string::npos );
    }

    SECTION( "no trees array" )
    {
        auto broken = json;
        broken.erase( "trees" );
        CHECK_FALSE( Project::FromJson( broken, &error ).has_value() );
        CHECK( error.find( "trees" ) != std::string::npos );
    }

    SECTION( "a tree entry with no graph" )
    {
        auto broken = json;
        broken["trees"][0].erase( "tree" );
        CHECK_FALSE( Project::FromJson( broken, &error ).has_value() );
        CHECK( error.find( "tree" ) != std::string::npos );
    }

    SECTION( "a graph that does not parse" )
    {
        auto broken = json;
        broken["trees"][0]["tree"]["nodes"][0]["type"] = "NoSuchNodeType";
        CHECK_FALSE( Project::FromJson( broken, &error ).has_value() );
        // The reason from the tree layer is passed through, not swallowed.
        CHECK( error.find( "NoSuchNodeType" ) != std::string::npos );
    }
}

TEST_CASE( "a project survives a trip through a file", "[project]" )
{
    ScratchFile scratch( "moth_noised_test_project.json" );

    Project project;
    project.trees.push_back( MakeTree( kCellularCaves, 12.0f, 34.0f ) );
    project.output = 0;
    project.preview.seed = 99;

    std::string error;
    INFO( "error: " << error );
    REQUIRE( project.Save( scratch.Path(), &error ) );
    REQUIRE( std::filesystem::exists( scratch.Path() ) );

    auto reloaded = Project::Load( scratch.Path(), &error );
    INFO( "error: " << error );
    REQUIRE( reloaded.has_value() );
    CHECK( reloaded->preview.seed == 99 );
    CHECK( reloaded->trees[0].tree.ToEncodedString() == project.trees[0].tree.ToEncodedString() );
    CHECK( reloaded->trees[0].layout[0].x == 12.0f );
}

TEST_CASE( "reading a file that is missing or malformed reports why", "[project]" )
{
    std::string error;

    SECTION( "missing" )
    {
        const auto missing = std::filesystem::temp_directory_path() / "moth_noised_no_such_project.json";
        std::filesystem::remove( missing );
        CHECK_FALSE( Project::Load( missing, &error ).has_value() );
        CHECK( error.find( "open" ) != std::string::npos );
    }

    SECTION( "not JSON" )
    {
        ScratchFile scratch( "moth_noised_test_garbage.json" );
        {
            std::ofstream file( scratch.Path() );
            file << "this is not json";
        }
        CHECK_FALSE( Project::Load( scratch.Path(), &error ).has_value() );
        CHECK( error.find( "parse" ) != std::string::npos );
    }
}
