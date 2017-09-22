#! /bin/sh

# Change directory
cd "$(dirname $0)"
# Configuration file
if [ -n "${CONFIG}" ]; then
    source "${CONFIG}.sh"
else
    echo "Error: Please pass configuration script by setting CONFIG variable!" >&2
    exit 1
fi

# Before uplink hook
before_upload_hook

# Create temporary directory
_TMP_REPO_DIR="$(mktemp -d)"
# Clone repository into temporary directory
git clone --no-checkout "${REPO_URL}" -b "${REPO_BRANCH}" "${_TMP_REPO_DIR}"
# Copy all files from source directory to repository
cp -a "${SOURCE_DIR}"/* "${_TMP_REPO_DIR}"

# Get reporitory version information
_REPO_VERSION_INFO="$(git describe --tags)"
# Add changes
git -C "${_TMP_REPO_DIR}" add .
# Commit changes
git -C "${_TMP_REPO_DIR}" commit -m "${_REPO_VERSION_INFO}"
# Push commit to remote
git -C "${_TMP_REPO_DIR}" push origin "${REPO_BRANCH}"

# Remove temporary directory
rm -rf "${_TMP_REPO_DIR}"

# After upload hook
after_upload_hook
