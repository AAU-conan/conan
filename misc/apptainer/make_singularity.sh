#! /usr/bin/bash
SHORT=$(git rev-parse --short HEAD)
SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
PROJECT_ROOT=$(dirname "$(dirname "$SCRIPT_DIR")")
PROJECT_NAME=$(basename "$PROJECT_ROOT")

CPLEX_BIND=""
if [[ $1 == "cplex" ]]; then
    echo "Building CPLEX image"
    APPTAINER_FILE=ApptainerCPLEX
    export SINGULARITYENV_cplex_DIR=/cplex
    CPLEX_BIND="--bind /opt/ibm/ILOG/CPLEX_Studio2211/cplex:/cplex"
else
    echo "Building normal image"
    APPTAINER_FILE=Apptainer
fi

# Make the singularity builds directory if it doesn't exist
mkdir -p builds/singularity_builds

# Build the base build image if the definition file has changed
if [[ "$SCRIPT_DIR/ApptainerBaseBuild" -nt "$SCRIPT_DIR/base_build.img" ]]; then
    sudo singularity build "$SCRIPT_DIR/base_build.img" "$SCRIPT_DIR/ApptainerBaseBuild"
fi

# Build the base run image if the definition file has changed
if [[ "$SCRIPT_DIR/ApptainerBaseRun" -nt "$SCRIPT_DIR/base_run.img" ]]; then
    sudo singularity build "$SCRIPT_DIR/base_run.img" "$SCRIPT_DIR/ApptainerBaseRun"
fi

# Build the apptainer image if the definition file has changed
sudo singularity build $CPLEX_BIND\
    --bind "$PROJECT_ROOT/src:/src" \
    --bind "$PROJECT_ROOT/builds/singularity_builds:/builds" \
    "$PROJECT_NAME-$SHORT.img" "$SCRIPT_DIR/$APPTAINER_FILE"
