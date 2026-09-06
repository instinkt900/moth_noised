#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <moth/noise/node_tree.h>

namespace moth_noised
{
    /// @brief Schema version stamped into every project this editor writes.
    inline constexpr int kProjectVersion = 1;

    /// @brief Value of the @c format field identifying a project document.
    inline constexpr std::string_view kProjectFormat = "moth.noise.project";

    /// @brief Where a node sits on the editor canvas.
    struct NodeLayout
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    /// @brief One graph, with the canvas positions of its nodes.
    ///
    /// The layout runs parallel to @c tree.GetNodes(): entry @c i positions node
    /// @c i of the graph. It is kept beside the tree rather than inside it so
    /// that what gets written under "tree" is an ordinary moth.noise.tree
    /// document, readable by an engine that has never heard of this editor.
    struct ProjectTree
    {
        moth::noise::NodeTree tree;
        std::vector<NodeLayout> layout;
    };

    /// @brief What the editor keeps about how a graph is being previewed.
    ///
    /// Not part of the graph, and of no interest to anything that only wants to
    /// generate noise from it.
    struct PreviewSettings
    {
        int seed = 1337;
        float scale = 2.5f;
        int genType = 0;
    };

    /**
     * @brief A saved editor workspace.
     *
     * The canvas is a forest, not a single graph: nodes with nothing feeding
     * off them are each the root of their own tree, and one of them is the
     * output that drives the preview and anything reading over IPC. A project
     * is therefore an array of trees and an index saying which is the output,
     * rather than one tree with scratch work discarded.
     *
     * Move-only, because a NodeTree is.
     */
    class Project
    {
    public:
        Project() = default;

        Project( Project&& ) noexcept = default;
        Project& operator=( Project&& ) noexcept = default;
        Project( const Project& ) = delete;
        Project& operator=( const Project& ) = delete;

        std::vector<ProjectTree> trees;
        PreviewSettings preview;

        /// Index into @c trees of the output graph, or -1 when there is none.
        int output = -1;

        /// @brief The output tree, or nullptr when the project has no output.
        const ProjectTree* GetOutput() const;

        nlohmann::json ToJson() const;

        /**
         * @brief Read a project document.
         *
         * A bare moth.noise.tree document is also accepted, and loads as a
         * one-tree project with no layout — so a graph exported for an engine
         * can be opened again here without a conversion step.
         *
         * @param error  Optional; receives a human-readable reason on failure.
         */
        static std::optional<Project> FromJson( const nlohmann::json& json, std::string* error = nullptr );

        /// @brief Read a project from disk. Reports why on failure.
        static std::optional<Project> Load( const std::filesystem::path& path, std::string* error = nullptr );

        /// @brief Write the project to disk. Reports why on failure.
        bool Save( const std::filesystem::path& path, std::string* error = nullptr ) const;
    };
}
