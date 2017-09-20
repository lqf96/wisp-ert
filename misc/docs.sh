# Repository URL
REPO_URL="ssh://github.com/lqf96/wisp-ert"
# Repository branch
REPO_BRANCH="gh-pages"

# Source
SOURCE_DIR="../.docs"

# Before upload hook
before_upload_hook() {
    # Create directory
    mkdir -p "${SOURCE_DIR}"
    # Generate documents
    #(cd ../client; doxygen)
    (cd ../server; doxygen)
}
# After upload hook
after_upload_hook() {
    # Remove directory
    rm -rf "${SOURCE_DIR}"
}
