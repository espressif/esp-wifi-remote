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
MASTER_VERSION="6.1"
MASTER_BRANCH="master"

# Specific version overrides
# Define specific SHAs for versions that need exact commits
# Format: "version:sha" pairs, one per line
# Example: "5.5:1234567890abcdef1234567890abcdef12345678"
SPECIFIC_VERSIONS=(
    # Example usage:
    # "5.3:e0991facf5ecb362af6aac1fae972139eb38d2e4"
    # "5.4:1bfe1595fa6bc64dcd5241047dc677cc194ecdaf"
    # "5.5:4b2b5d7baf92473bf2fc881081507dbbbb7362e1"
    # "6.0:800f141f94c0f880c162de476512e183df671307"
)

# Default to processing both branches and tags
VERSION_TYPE="all"
SINGLE_VERSION=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --type)
            VERSION_TYPE="$2"
            shift 2
            ;;
        --version)
            SINGLE_VERSION="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--type {all|branches|tags}] [--version <version_dir>]"
            echo "  --type: Process all, branches only, or tags only"
            echo "  --version: Process only the specified version directory (e.g., idf_v5.5, idf_tag_v5.4.1)"
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

# Function to get specific SHA for a version
get_specific_sha() {
    local version=$1
    for entry in "${SPECIFIC_VERSIONS[@]}"; do
        if [[ "$entry" =~ ^[[:space:]]*# ]]; then
            # Skip commented lines
            continue
        fi
        if [[ "$entry" =~ ^[[:space:]]*$ ]]; then
            # Skip empty lines
            continue
        fi
        local entry_version="${entry%%:*}"
        local entry_sha="${entry##*:}"
        if [[ "$version" == "$entry_version" ]]; then
            echo "$entry_sha"
            return 0
        fi
    done
    return 1
}

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

            # Check if this version has a specific SHA override
            local specific_sha
            if specific_sha=$(get_specific_sha "$version_num"); then
                echo "Using specific SHA for version $version_num: $specific_sha"
                echo "Fetching from remote to ensure we have the commit..."
                git fetch "$REMOTE" "$branch"
                git checkout "$specific_sha"
            else
                echo "Checking out branch: $branch"
                git fetch "$REMOTE" "$branch"
                git checkout "$REMOTE/$branch"
            fi
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
if [ -n "$SINGLE_VERSION" ]; then
    echo "Starting generation for single version: $SINGLE_VERSION"
    cd "$IDF_PATH"

    # Check if the specified version directory exists
    version_dir="$WIFI_REMOTE_DIR/$SINGLE_VERSION"
    if [ -d "$version_dir" ]; then
        process_version "$version_dir"
        echo "Version $SINGLE_VERSION processed successfully!"
    else
        echo "Error: Version directory '$version_dir' does not exist"
        echo "Available version directories:"
        for dir in "$WIFI_REMOTE_DIR"/idf_*; do
            if [ -d "$dir" ]; then
                echo "  $(basename "$dir")"
            fi
        done
        exit 1
    fi
else
    echo "Starting generation for all versions..."
    cd "$IDF_PATH"

    # Find all version directories
    for version_dir in "$WIFI_REMOTE_DIR"/idf_*; do
        if [ -d "$version_dir" ]; then
            process_version "$version_dir"
        fi
    done

    echo "All versions processed successfully!"
fi
