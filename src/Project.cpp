#include "Project.h"

#include <algorithm>
#include <fstream>

namespace moth_noised
{
    namespace
    {
        /// Assigns *error when the caller asked for one, then reports failure.
        bool Fail( std::string* error, std::string message )
        {
            if( error )
            {
                *error = std::move( message );
            }
            return false;
        }

        /// Reads one tree entry. Layout entries beyond the node count are
        /// dropped and missing ones default, so a file hand-edited to add a
        /// node still opens — the graph is what matters, positions are a
        /// convenience the editor can recompute.
        bool ReadTree( const nlohmann::json& entry, size_t index, ProjectTree& out, std::string* error )
        {
            if( !entry.is_object() )
            {
                return Fail( error, "tree " + std::to_string( index ) + " is not an object" );
            }

            const auto treeIt = entry.find( "tree" );
            if( treeIt == entry.end() )
            {
                return Fail( error, "tree " + std::to_string( index ) + " has no 'tree' document" );
            }

            std::string treeError;
            auto parsed = moth::noise::NodeTree::FromJson( *treeIt, &treeError );
            if( !parsed )
            {
                return Fail( error, "tree " + std::to_string( index ) + ": " + treeError );
            }
            out.tree = std::move( *parsed );

            out.layout.assign( out.tree.GetNodeCount(), NodeLayout {} );
            const auto layoutIt = entry.find( "layout" );
            if( layoutIt != entry.end() && layoutIt->is_array() )
            {
                const size_t count = std::min( layoutIt->size(), out.layout.size() );
                for( size_t i = 0; i < count; ++i )
                {
                    const auto& position = ( *layoutIt )[i];
                    if( position.is_object() )
                    {
                        out.layout[i].x = position.value( "x", 0.0f );
                        out.layout[i].y = position.value( "y", 0.0f );
                    }
                }
            }

            return true;
        }
    }

    const ProjectTree* Project::GetOutput() const
    {
        if( output < 0 || static_cast<size_t>( output ) >= trees.size() )
        {
            return nullptr;
        }
        return &trees[static_cast<size_t>( output )];
    }

    nlohmann::json Project::ToJson() const
    {
        nlohmann::json out;
        out["format"] = std::string( kProjectFormat );
        out["version"] = kProjectVersion;
        out["output"] = GetOutput() ? nlohmann::json( output ) : nlohmann::json( nullptr );

        auto entries = nlohmann::json::array();
        for( const auto& tree : trees )
        {
            auto layout = nlohmann::json::array();
            // Only as many positions as there are nodes: the layout is written
            // to match what the tree document actually contains, which may be
            // fewer nodes than the editor had if some were unreachable.
            const size_t count = std::min( tree.layout.size(), tree.tree.GetNodeCount() );
            for( size_t i = 0; i < count; ++i )
            {
                layout.push_back( { { "x", tree.layout[i].x }, { "y", tree.layout[i].y } } );
            }

            entries.push_back( { { "tree", tree.tree.ToJson() }, { "layout", std::move( layout ) } } );
        }
        out["trees"] = std::move( entries );

        out["preview"] = {
            { "seed", preview.seed },
            { "scale", preview.scale },
            { "type", preview.genType },
        };

        return out;
    }

    std::optional<Project> Project::FromJson( const nlohmann::json& json, std::string* error )
    {
        if( !json.is_object() )
        {
            Fail( error, "project JSON must be an object" );
            return std::nullopt;
        }

        const auto format = json.value( "format", std::string {} );

        // A bare graph opens as a one-tree project. The editor writes projects,
        // but what it hands an engine is a tree, and being unable to reopen your
        // own export would be a poor trade for a nesting rule.
        if( format == moth::noise::kNodeTreeFormat )
        {
            std::string treeError;
            auto tree = moth::noise::NodeTree::FromJson( json, &treeError );
            if( !tree )
            {
                Fail( error, treeError );
                return std::nullopt;
            }

            Project project;
            ProjectTree entry;
            entry.layout.assign( tree->GetNodeCount(), NodeLayout {} );
            entry.tree = std::move( *tree );
            project.trees.push_back( std::move( entry ) );
            project.output = 0;
            return project;
        }

        if( format != kProjectFormat )
        {
            Fail( error, "unexpected format '" + format + "', expected '" + std::string( kProjectFormat ) + "'" );
            return std::nullopt;
        }

        const auto version = json.value( "version", -1 );
        if( version != kProjectVersion )
        {
            Fail( error, "unsupported project version " + std::to_string( version ) + ", this build reads version " + std::to_string( kProjectVersion ) );
            return std::nullopt;
        }

        const auto treesIt = json.find( "trees" );
        if( treesIt == json.end() || !treesIt->is_array() )
        {
            Fail( error, "project has no 'trees' array" );
            return std::nullopt;
        }

        Project project;
        project.trees.reserve( treesIt->size() );
        for( size_t i = 0; i < treesIt->size(); ++i )
        {
            ProjectTree entry;
            if( !ReadTree( ( *treesIt )[i], i, entry, error ) )
            {
                return std::nullopt;
            }
            project.trees.push_back( std::move( entry ) );
        }

        const auto outputIt = json.find( "output" );
        if( outputIt != json.end() && outputIt->is_number_integer() )
        {
            const auto index = outputIt->get<int>();
            if( index < 0 || static_cast<size_t>( index ) >= project.trees.size() )
            {
                Fail( error, "'output' index " + std::to_string( index ) + " is out of range" );
                return std::nullopt;
            }
            project.output = index;
        }

        const auto previewIt = json.find( "preview" );
        if( previewIt != json.end() && previewIt->is_object() )
        {
            project.preview.seed = previewIt->value( "seed", project.preview.seed );
            project.preview.scale = previewIt->value( "scale", project.preview.scale );
            project.preview.genType = previewIt->value( "type", project.preview.genType );
        }

        return project;
    }

    std::optional<Project> Project::Load( const std::filesystem::path& path, std::string* error )
    {
        std::ifstream file( path );
        if( !file )
        {
            Fail( error, "could not open " + path.string() );
            return std::nullopt;
        }

        nlohmann::json json;
        try
        {
            file >> json;
        }
        catch( const nlohmann::json::exception& e )
        {
            Fail( error, std::string( "could not parse " ) + path.string() + ": " + e.what() );
            return std::nullopt;
        }

        return FromJson( json, error );
    }

    bool Project::Save( const std::filesystem::path& path, std::string* error ) const
    {
        std::error_code ec;
        if( path.has_parent_path() )
        {
            std::filesystem::create_directories( path.parent_path(), ec );
        }

        std::ofstream file( path );
        if( !file )
        {
            return Fail( error, "could not write " + path.string() );
        }

        file << ToJson().dump( 4 ) << '\n';
        if( !file )
        {
            return Fail( error, "could not write " + path.string() );
        }

        return true;
    }
}
