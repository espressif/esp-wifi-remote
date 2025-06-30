#!/bin/bash

# Exit on any error
set -e

# Configuration
IDF_PATH=${IDF_PATH:-""}
REMOTE="github"
WIFI_REMOTE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPTS_DIR="${WIFI_REMOTE_DIR}/scripts"

# Master branch configuration
# Update this when a new major version is released
MASTER_VERSION="6.0"
MASTER_BRANCH="master"

# Default to processing both branches and tags
VERSION_TYPE="all"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --type)
            VERSION_TYPE="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--type {all|branches|tags}]"
            exit 1
            ;;
    esac
done

# Validate version type
if [[ ! "$VERSION_TYPE" =~ ^(all|branches|tags)$ ]]; then
    echo "Error: Invalid version type. Must be one of: all, branches, tags"
    exit 1
fi

# Check if IDF_PATH is set
if [ -z "$IDF_PATH" ]; then
    echo "Error: IDF_PATH environment variable is not set"
    exit 1
fi

# Function to process a version
process_version() {
    local version_dir=$1
    local version_name=$(basename "$version_dir")

    echo "Processing version: $version_name"

    # Create a new subshell for processing this version
    (
        # Determine if this is a branch or tag version
        if [[ $version_name =~ ^idf_v[0-9]+\.[0-9]+$ ]]; then
            # Skip if we're only processing tags
            if [[ "$VERSION_TYPE" == "tags" ]]; then
                echo "Skipping branch version (tags only mode): $version_name"
                return
            fi
            # Branch version (e.g., idf_v5.4)
            local version_num="${version_name#idf_v}"
            local branch
            if [[ "$version_num" == "$MASTER_VERSION" ]]; then
                branch="$MASTER_BRANCH"
                echo "Using master branch for version $version_num"
            else
                branch="release/v$version_num"
            fi
            echo "Checking out branch: $branch"
            git fetch "$REMOTE" "$branch"
            git checkout "$REMOTE/$branch"
        elif [[ $version_name =~ ^idf_tag_v[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
            # Skip if we're only processing branches
            if [[ "$VERSION_TYPE" == "branches" ]]; then
                echo "Skipping tag version (branches only mode): $version_name"
                return
            fi
            # Tag version (e.g., idf_tag_v5.4.1 or idf_tag_v5.4)
            local tag="${version_name#idf_tag_}"
            echo "Checking out tag: $tag"
            git fetch "$REMOTE" "$tag" --tags
            git checkout "$tag"
        else
            echo "Skipping unknown version format: $version_name"
            return
        fi

        # Update submodules
        echo "Updating submodules..."
        git submodule update --init --recursive --force

        # Install and export ESP-IDF
        echo "Installing ESP-IDF..."
        ./install.sh
        source export.sh
        pip install idf_build_apps

        # Generate and check
        echo "Generating files..."
        cd "$SCRIPTS_DIR"
        python generate_and_check.py
    )
}

# Main execution
echo "Starting generation for all versions..."
cd "$IDF_PATH"

# Find all version directories
for version_dir in "$WIFI_REMOTE_DIR"/idf_*; do
    if [ -d "$version_dir" ]; then
        process_version "$version_dir"
    fi
done

echo "All versions processed successfully!"
