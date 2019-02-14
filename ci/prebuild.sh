#!/usr/bin/env bash
# Prepares a directory to build Micro-Manager for the Raspberry Pi.
#
# USAGE: ./prebuild.sh SRC_DIR
#
# SRC_DIR is the absolute path to the directory that will contain the
# Micro-Manager source code and 3rdpartypublic repository.
#
# Kyle M. Douglass, 2018
# kyle.m.douglass@gmail.com
#

set -euo pipefail

BASE_DIR=$(dirname "$0")
SRC_DIR=$1
MM_SRC_DIR=$SRC_DIR/micro-manager
MM_GIT_URL="https://github.com/nicost/micro-manager.git"
MM_GIT_BRANCH="ViewerPlusCV"
SVN_URL="https://valelab4.ucsf.edu/svn/3rdpartypublic/"
DEP_DIR=$SRC_DIR/3rdpartypublic

function get_mm_source {
    echo "Checking out the latest Micro-Manager source code..."
    [[ -d $MM_SRC_DIR/.git ]] || git clone "$MM_GIT_URL" "$MM_SRC_DIR"
    git --git-dir="$MM_SRC_DIR"/.git --work-tree="$MM_SRC_DIR" checkout "$MM_GIT_BRANCH"
    git --git-dir="$MM_SRC_DIR"/.git --work-tree="$MM_SRC_DIR" pull origin "$MM_GIT_BRANCH"
    echo -e "Done checking out the Micro-Manager source code.\n"
}

function get_3rdpartypublic {
    echo "Checking out the 3rdpartypublic dependencies. This can take a while..."
    svn co "$SVN_URL" "$DEP_DIR" > /dev/null 2>&1
    while [ $? -ne 0 ]; do :
	# Continues the checkout in case timeout errors occur
        echo "Still working..."
        svn cleanup "$DEP_DIR" > /dev/null
        svn co "$SVN_URL" "$DEP_DIR" > /dev/null 2>&1
    done
    echo -e "Done checking out the 3rdpartypublic dependencies.\n"
}

function merge_directories {
    echo "Merging device adapter files with Micro-Manager source..."
    "$BASE_DIR"/merge.sh "$SRC_DIR"
    
    echo -e "Done merging with Micro-Manager source.\n"
}

mkdir -p "$SRC_DIR" "$HOME"/.ccache
get_mm_source

# Unset error checking to prevent svn timeout errors
set +e
get_3rdpartypublic

set -e
merge_directories
